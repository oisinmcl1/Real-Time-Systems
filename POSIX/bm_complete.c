#define _GNU_SOURCE  /* MUST be defined before including headers */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#define ITERATIONS 1000 // Number of iterations for nanosleep & signal benchmarks
#define TIMER_ITERATIONS 10000 // Number of iterations for timer benchmark
#define NS_PER_SEC 1000000000LL
#define NS_PER_MSEC 1000000L


// Globals
// volatile sig_atomic_t signal_received = 0; // I do not believe this is used anymore after sigwait approach
struct timespec start, end, sleep_time;

timer_t timer_id;
long long timer_ar[TIMER_ITERATIONS];
int timer_cnt = 0;

// System configuration helpers
void configure_realtime_scheduling() {
    /*
     * Configure the process to use real-time scheduling with the highest priority.
     * Uses SCHED_FIFO, which is a first-in-first-out real-time scheduling policy.
     */
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler");
        exit(EXIT_FAILURE);
    }
}

void lock_memory() {
    /*
     * Lock the process's memory to prevent it from being swapped out.
     * Important for real-time performance, as swapping can introduce significant latency.
     */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("mlockall");
        exit(EXIT_FAILURE);
    }
}

void set_affinity() {
    /*
     * Pin the process to a single CPU (CPU 0).
     * Reduces scheduling variability and helps achieve more consistent timing results.
     */
    cpu_set_t mask; // CPU affinity mask
    CPU_ZERO(&mask); // Clear the CPU mask
    CPU_SET(0, &mask); // Set CPU 0 in the mask
    sched_setaffinity(0, sizeof(mask), &mask); // Set the CPU affinity
}

// This function is no longer used after sigwait approach
// void signal_handler(int signum) {
//     /*
//      * Called when SIGUSR1 is received. Sets the signal_received flag
//      * and records the time at which the signal was handled.
//      */
//     signal_received = 1;
//     clock_gettime(CLOCK_MONOTONIC, &end); // Record the end time
// }


/* 
 * Benchmark 1: nanosleep jitter
 */
void benchmark_nanosleep() {
    long long total_jitter = 0;
    long long max_jitter   = 0;
    long long min_jitter   = LLONG_MAX;

    sleep_time.tv_sec  = 0;
    sleep_time.tv_nsec = 1000000; // 1 ms

    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start); // Start time
        nanosleep(&sleep_time, NULL); // Sleep for 1 ms
        clock_gettime(CLOCK_MONOTONIC, &end); // Record the end time

        // Calculate elapsed time and jitter
        long long elapsed = (end.tv_sec  - start.tv_sec) * NS_PER_SEC + (end.tv_nsec - start.tv_nsec);
        long long jitter  = elapsed - sleep_time.tv_nsec;

        total_jitter += llabs(jitter);
        if (jitter > max_jitter) max_jitter = jitter;
        if (jitter < min_jitter) min_jitter = jitter;
    }

    printf("Nanosleep Benchmark (%d iterations):\n", ITERATIONS);
    printf("    Average jitter: %lld ns\n", total_jitter / ITERATIONS);
    printf("    Max jitter: %lld ns\n", max_jitter);
    printf("    Min jitter: %lld ns\n", min_jitter);
}


/*
 * Benchmark 2: signal delivery latency
 */
void benchmark_signal_latency() {
    long long total_latency = 0;
    long long max_latency   = 0;
    long long min_latency   = LLONG_MAX;

    // signal(SIGUSR1, signal_handler); // Set up the signal handler for SIGUSR1

    // Block SIGUSR1
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start); // Start time

        kill(getpid(), SIGUSR1); // Send SIGUSR1
        // while (!signal_received); // Spin until the signal has been handled

        int sig;
        sigwait(&mask, &sig); // Wait for the signal
        
        clock_gettime(CLOCK_MONOTONIC, &end); // Record the end time

        // Record the end time and calculate latency
        long long latency = (end.tv_sec  - start.tv_sec) * NS_PER_SEC + (end.tv_nsec - start.tv_nsec);

        total_latency += latency;
        if (latency > max_latency) max_latency = latency;
        if (latency < min_latency) min_latency = latency;

        // signal_received = 0; // Reset the signal received flag
    }

    printf("\nSignal Latency Benchmark (%d iterations):\n", ITERATIONS);
    printf("    Average latency: %lld ns\n", total_latency / ITERATIONS);
    printf("    Max latency: %lld ns\n", max_latency);
    printf("    Min latency: %lld ns\n", min_latency);
}

/*
 * Benchmark 3: POSIX timer jitter
 */
void benchmark_timer() {
    long long total_jitter = 0;
    long long max_jitter = 0;
    long long min_jitter = LLONG_MAX;
    long long elapsed;
    long long jitter;

    // Set up the signal event for the timer
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timer_id;

    // Create the timer
    if (timer_create(CLOCK_MONOTONIC, &sev, &timer_id) == -1) {
        perror("timer_create");
        exit(EXIT_FAILURE);
    }

    // Configure the timer to fire every 1 ms
    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = NS_PER_MSEC;
    its.it_interval = its.it_value;

    // Start the timer
    if (timer_settime(timer_id, 0, &its, NULL) == -1) {
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }

    // Block SIGRTMIN so we can use sigwait() for efficient waiting
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGRTMIN);
    sigprocmask(SIG_BLOCK, &mask, NULL); // Block SIGRTMIN so we can wait for it

    clock_gettime(CLOCK_MONOTONIC, &start); // Get the start time
    for (int i = 0; i < TIMER_ITERATIONS; i++) {
        int sig;
        sigwait(&mask, &sig); // Wait for the timer signal
        clock_gettime(CLOCK_MONOTONIC, &end); // Get the end time

        // Calculate elapsed time and jitter - I think orignal approach ignored seconds
        // if ((end.tv_nsec - start.tv_nsec) < 0) {
        //     elapsed = end.tv_nsec - start.tv_nsec + NS_PER_SEC;
        // } else {
        //     elapsed = end.tv_nsec - start.tv_nsec;
        // }
        elapsed = (end.tv_sec - start.tv_sec) * NS_PER_SEC + (end.tv_nsec - start.tv_nsec);

        jitter = elapsed - NS_PER_MSEC;

        total_jitter += llabs(jitter);
        if (jitter > max_jitter) max_jitter = jitter;
        if (jitter < min_jitter) min_jitter = jitter;
        timer_ar[timer_cnt++] = jitter;
        start = end; // Update start time for next iteration
    }

    // Print individual jitter values
    // for (int j = 0; j < TIMER_ITERATIONS; j++) printf("%lld\t", timer_ar[j]);

    printf("\nTimer Benchmark (%d iterations):\n", TIMER_ITERATIONS);
    printf("    Average jitter: %lld ns\n", total_jitter / TIMER_ITERATIONS);
    printf("    Max jitter: %lld ns\n", max_jitter);
    printf("    Min jitter: %lld ns\n", min_jitter);

    timer_delete(timer_id);
}


int main() {
    configure_realtime_scheduling();
    lock_memory();
    set_affinity();

    benchmark_nanosleep();
    benchmark_signal_latency();
    benchmark_timer();

    return 0;
}
