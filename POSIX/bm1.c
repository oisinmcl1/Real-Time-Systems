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

#define ITERATIONS 1000
#define NS_PER_SEC 1000000000L

volatile sig_atomic_t signal_received = 0;
struct timespec start, end, sleep_time;

void signal_handler(int signum) {
    signal_received = 1;
	clock_gettime(CLOCK_MONOTONIC, &end);
}

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

void benchmark_nanosleep() {
    // struct timespec start, end, sleep_time;
    long long total_jitter = 0;
    long long max_jitter = 0;
    long long min_jitter = LLONG_MAX;

    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 1000000; // 1 ms

    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        nanosleep(&sleep_time, NULL);
        clock_gettime(CLOCK_MONOTONIC, &end);

        long long elapsed = (end.tv_sec - start.tv_sec) * NS_PER_SEC + (end.tv_nsec - start.tv_nsec);
        long long jitter = elapsed - sleep_time.tv_nsec;

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

    signal(SIGUSR1, signal_handler);

    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        kill(getpid(), SIGUSR1);
        while (!signal_received);

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