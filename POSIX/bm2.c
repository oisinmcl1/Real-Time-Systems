// Compile code with gcc -o bm2 bm2.c -lrt -Wall -O2
// Execute code with sudo ./bm2

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

#define ITERATIONS 10000
#define NS_PER_SEC 1000000000LL
#define NS_PER_MSEC 1000000L


timer_t timer_id;
struct timespec start, end;
long long ar[ITERATIONS];
int cnt = 0;

void configure_realtime_scheduling() {
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler");
        exit(EXIT_FAILURE);
    }
}

void lock_memory() {
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("mlockall");
        exit(EXIT_FAILURE);
    }
}

void set_affinity() {
    /*
     * This function sets the CPU affinity of the process to a single CPU (CPU 0 in this case).
     * This is done to reduce variability in scheduling and ensure that the process runs on the same
     * CPU, which can help in achieving more consistent timing results.
     */
    cpu_set_t mask; // Define a CPU set to specify the CPUs on which the process can run
    CPU_ZERO(&mask); // Initialize the CPU set to be empty
    CPU_SET(0, &mask); // Add CPU 0 to the set, allowing the process to run only on CPU 0
    sched_setaffinity(0, sizeof(mask), &mask); // Set the CPU affinity for the current process (PID 0) to the specified CPU set
}

void benchmark_timer() {
    long long total_jitter = 0;
    long long max_jitter = 0;
    long long min_jitter = LLONG_MAX;
    long long elapsed;
    long long jitter;

    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timer_id;

    // Create the timer
    if (timer_create(CLOCK_MONOTONIC, &sev, &timer_id) == -1) {
        perror("timer_create");
        exit(EXIT_FAILURE);
    }

    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = NS_PER_MSEC; // 1 ms
    its.it_interval = its.it_value;

    // Start the timer
    if (timer_settime(timer_id, 0, &its, NULL) == -1) {
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }

    // Use a dedicated signal set to wait for the timer
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGRTMIN);
    sigprocmask(SIG_BLOCK, &mask, NULL); // Block it so we can wait for it

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ITERATIONS; i++) {
      // This suspends the process efficiently until the signal arrives
      int sig;
      sigwait(&mask, &sig);
      clock_gettime(CLOCK_MONOTONIC, &end);

      if ((end.tv_nsec - start.tv_nsec) < 0) {
        elapsed = end.tv_nsec - start.tv_nsec + NS_PER_SEC;
      } else {
        elapsed = end.tv_nsec - start.tv_nsec;
      }

      jitter = elapsed - NS_PER_MSEC;

      total_jitter += llabs(jitter);
      if (jitter > max_jitter) max_jitter = jitter;
      if (jitter < min_jitter) min_jitter = jitter;
      ar[cnt++] = jitter;
      start = end;
    }

    for (int j = 0; j < ITERATIONS; j++) printf("%lld\t", ar[j]);

    printf("\nTimer Benchmark:\n");
    printf("Average jitter: %lld ns\n", total_jitter / ITERATIONS);
    printf("    Max jitter: %lld ns\n", max_jitter);
    printf("    Min jitter: %lld ns\n", min_jitter);

    timer_delete(timer_id);
}

int main() {
    configure_realtime_scheduling();
    lock_memory();
    set_affinity();

    benchmark_timer();

    return 0;
}
