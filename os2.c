#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#define N 3
#define TARGET_CPU_MS 1000   
// Time measurement function
double now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}
// Time measurement for the child process
double cpu_ms_child() {
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

void child_work() {
    raise(SIGSTOP); 
    volatile unsigned long x = 1;
    double start_cpu = cpu_ms_child();
    // Perform CPU-bound work
    while ((cpu_ms_child() - start_cpu) < TARGET_CPU_MS) {
        x = x * 1103515245u + 12345u;
        x ^= (x >> 11);
    }
    _exit(0);
}
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <quantum_ms>\n", argv[0]);
        return 1;
    }

    int Q = atoi(argv[1]); // 2 / 9 / 26
    pid_t pids[N];
    int alive[N] = {0};
    int slices[N] = {0};
    double response[N], turnaround[N];
    char path[256];
    snprintf(path, sizeof(path),
             "%s/Desktop/result_q%d.txt", getenv("HOME"), Q);

    FILE *fp = fopen(path, "w");
    if (!fp) { perror("file"); return 1; }

    fprintf(fp, "--- Round Robin Experiment (N=%d, Q=%d) ---\n", N, Q);
    printf ("--- Round Robin Experiment (N=%d, Q=%d) ---\n", N, Q);

    // fork children process
    for (int i = 0; i < N; i++) {
        pid_t pid = fork();
        if (pid == 0) child_work();
        pids[i] = pid;
        alive[i] = 1;
        response[i] = -1;
    }
    for (int i = 0; i < N; i++) {
        int st;
        waitpid(pids[i], &st, WUNTRACED);
    }
    double t0 = now_ms();
    int remaining = N;
    int idx = 0;
    int total_ctx = 0;

    while (remaining > 0) {
        if (!alive[idx]) {
            idx = (idx + 1) % N;
            continue;
        }
        if (response[idx] < 0)
            response[idx] = now_ms() - t0;

        kill(pids[idx], SIGCONT);
        slices[idx]++;
        total_ctx++;
        struct timespec req = {
            .tv_sec = Q / 1000,
            .tv_nsec = (Q % 1000) * 1000000L
        };
        nanosleep(&req, NULL);
        int status;
        pid_t w = waitpid(pids[idx], &status, WNOHANG);

        if (w == 0) {
            kill(pids[idx], SIGSTOP);
            waitpid(pids[idx], &status, WUNTRACED);
        } else {
            turnaround[idx] = now_ms() - t0;
            alive[idx] = 0;
            remaining--;
        }
        idx = (idx + 1) % N;
    }
    double total_time = now_ms() - t0;
    double avg_r = 0, avg_t = 0;

    for (int i = 0; i < N; i++) {
        avg_r += response[i];
        avg_t += turnaround[i];

        fprintf(fp,
            "[Process %d] Started at: %.4f ms\n"
            "[Process %d] Finished at: %.4f ms | Turnaround: %.4f ms | Response: %.4f ms | Slices: %d\n",
            i, response[i],
            i, turnaround[i], turnaround[i], response[i], slices[i]);

        printf(
            "[Process %d] Started at: %.4f ms\n"
            "[Process %d] Finished at: %.4f ms | Turnaround: %.4f ms | Response: %.4f ms | Slices: %d\n",
            i, response[i],
            i, turnaround[i], turnaround[i], response[i], slices[i]);
    }
    avg_r /= N;
    avg_t /= N;
    fprintf(fp,
        "\nTotal Execution Time: %.4f ms\n"
        "Average Response Time: %.4f ms\n"
        "Average Turnaround Time: %.4f ms\n"
        "Estimated Context Switches (Total): %d\n",
        total_time, avg_r, avg_t, total_ctx);

    printf(
        "\nTotal Execution Time: %.4f ms\n"
        "Average Response Time: %.4f ms\n"
        "Average Turnaround Time: %.4f ms\n"
        "Estimated Context Switches (Total): %d\n",
        total_time, avg_r, avg_t, total_ctx);

    fclose(fp);
    return 0;
}
