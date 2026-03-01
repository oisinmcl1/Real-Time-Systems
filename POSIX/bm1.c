// Compile code with gcc -o bm1 bm1.c -lrt -Wall -O2
// Execute code with sudo ./bm1

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

#define ITERATIONS 1000 // Number of iterations for benchmarking
#define NS_PER_SEC 1000000000L // Nanoseconds per second

volatile sig_atomic_t signal_received = 0; // Flag to indicate signal reception
struct timespec start, end, sleep_time; // Global variables for timing and sleep duration

void signal_handler(int signum) {
    /*
     * This handler is called when SIGUSR1 is received. It sets the signal_received flag
     * and records the time at which the signal was handled.
     */
    signal_received = 1;
	clock_gettime(CLOCK_MONOTONIC, &end);
}

void configure_realtime_scheduling() {
    /*
     * This function configures the process to use real-time scheduling with the highest priority.
     * It uses the SCHED_FIFO policy, which is a first-in-first-out real-time scheduling policy.
     */
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO); // Get the maximum priority for SCHED_FIFO
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) { // Set the scheduler for the current process
        perror("sched_setscheduler");
        exit(EXIT_FAILURE);
    }
}

void lock_memory() {
    /*
     * This function locks the process's memory to prevent it from being swapped out.
     * This is important for real-time performance, as swapping can introduce significant latency.
     */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) { // Lock all current and future memory
        perror("mlockall");
        exit(EXIT_FAILURE);
    }
}

void benchmark_nanosleep() {
    // struct timespec start, end, sleep_time;
    long long total_jitter = 0;
    long long max_jitter = 0;
    long long min_jitter = LLONG_MAX;

    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 1000000; // 1 ms

    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start); // Record the start time
        nanosleep(&sleep_time, NULL); // Sleep for the specified duration
        clock_gettime(CLOCK_MONOTONIC, &end); // Record the end time

        // Calculate the elapsed time and jitter
        long long elapsed = (end.tv_sec - start.tv_sec) * NS_PER_SEC + (end.tv_nsec - start.tv_nsec);
        long long jitter = elapsed - sleep_time.tv_nsec;

        // Update total, max, and min jitter
        total_jitter += llabs(jitter);
        if (jitter > max_jitter) max_jitter = jitter;
        if (jitter < min_jitter) min_jitter = jitter;
    }

    printf("Nanosleep Benchmark:\n");
    printf("Average jitter: %lld ns\n", total_jitter / ITERATIONS);
    printf("Max jitter: %lld ns\n", max_jitter);
    printf("Min jitter: %lld ns\n", min_jitter);
}

void benchmark_signal_latency() {
    long long total_latency = 0;
    long long max_latency = 0;
    long long min_latency = LLONG_MAX;

    signal(SIGUSR1, signal_handler); // Set up the signal handler for SIGUSR1

    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        kill(getpid(), SIGUSR1); // Send SIGUSR1 to the current process to trigger the signal handler
        while (!signal_received); // Wait until the signal is received and handled

        // Calculate the latency from sending the signal to handling it
        long long latency = (end.tv_sec - start.tv_sec) * NS_PER_SEC + (end.tv_nsec - start.tv_nsec);
        total_latency += latency;
        if (latency > max_latency) max_latency = latency;
        if (latency < min_latency) min_latency = latency;

        signal_received = 0;
    }

    printf("\nSignal Latency Benchmark:\n");
    printf("Average latency: %lld ns\n", total_latency / ITERATIONS);
    printf("Max latency: %lld ns\n", max_latency);
    printf("Min latency: %lld ns\n", min_latency);
}

int main() {
    configure_realtime_scheduling();
    lock_memory();

    benchmark_nanosleep();
    benchmark_signal_latency();

    return 0;
}