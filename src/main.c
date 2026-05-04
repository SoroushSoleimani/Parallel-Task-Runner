#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include "process_pool.h"
#include "log_manager.h"

#define MAX_CMD_LEN 1024
#define MAX_PIDS 1000

void execute_command(const char *command, int pipe_write_end, double timeout_sec) {
    if (dup2(pipe_write_end, STDOUT_FILENO) == -1) {
        perror("dup2 stdout");
        exit(EXIT_FAILURE);
    }
    if (dup2(pipe_write_end, STDERR_FILENO) == -1) {
        perror("dup2 stderr");
        exit(EXIT_FAILURE);
    }
    close(pipe_write_end);

    if (timeout_sec > 0) {
        alarm(timeout_sec);   
    }

    char *cmd_copy = strdup(command);
    if (!cmd_copy) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    char *args[MAX_CMD_LEN];
    int arg_count = 0;
    char *token = strtok(cmd_copy, " \t\n");
    while (token != NULL && arg_count < MAX_CMD_LEN - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[arg_count] = NULL;
    execvp(args[0], args);
    fprintf(stderr, "Command not found: %s\n", args[0]);
    free(cmd_copy);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <max_concurrent> <commands_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int max_concurrent = atoi(argv[1]);
    if (max_concurrent <= 0) {
        fprintf(stderr, "Error: max_concurrent must be positive\n");
        exit(EXIT_FAILURE);
    }
    FILE *input_file = fopen(argv[2], "r");
    if (!input_file) {
        perror("fopen input file");
        exit(EXIT_FAILURE);
    }
    FILE *log_file = open_log_file("output.log");
    if (!log_file) {
        perror("fopen output.log");
        fclose(input_file);
        exit(EXIT_FAILURE);
    }

    double timeout_sec = 0;
    char *timeout_env = getenv("TIMEOUT");
    if (timeout_env != NULL) {
        timeout_sec = atof(timeout_env);
        if (timeout_sec > 0) {
            printf("Timeout enabled: %.1f seconds per command (using alarm)\n", timeout_sec);
        }
    }

    struct timespec prog_start, prog_end;
    clock_gettime(CLOCK_MONOTONIC, &prog_start);

    child_process_t children[MAX_PIDS];
    int active_count = 0;
    char command[MAX_CMD_LEN];
    bool end_of_file = false;

    int total_commands = 0;
    int success_count = 0;
    int fail_count = 0;

    while (1) {
        if (!end_of_file && active_count < max_concurrent) {
            if (fgets(command, sizeof(command), input_file) != NULL) {
                command[strcspn(command, "\n")] = '\0';
                if (command[0] == '\0') continue;

                total_commands++;

                int pipe_fds[2];
                if (pipe(pipe_fds) == -1) {
                    perror("pipe");
                    continue;
                }
                pid_t pid = fork();
                if (pid == -1) {
                    perror("fork");
                    close(pipe_fds[0]);
                    close(pipe_fds[1]);
                    continue;
                } else if (pid == 0) {
                    // فرزند
                    close(pipe_fds[0]);
                    execute_command(command, pipe_fds[1], timeout_sec);
                    exit(EXIT_FAILURE);
                } else {
                    // والد
                    close(pipe_fds[1]);
                    children[active_count].pid = pid;
                    children[active_count].pipe_fd = pipe_fds[0];
                    children[active_count].exit_status = -1;
                    active_count++;
                }
            } else {
                end_of_file = true;
            }
        }

        reap_finished_processes(children, &active_count, log_file, &success_count, &fail_count);

        if (end_of_file && active_count == 0) break;
        if (!end_of_file && active_count == max_concurrent) {
            usleep(10000);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &prog_end);
    double elapsed = (prog_end.tv_sec - prog_start.tv_sec) +
                     (prog_end.tv_nsec - prog_start.tv_nsec) / 1e9;

    printf("\n========== EXECUTION REPORT ==========\n");
    printf("Total commands:     %d\n", total_commands);
    printf("Successful:         %d\n", success_count);
    printf("Failed:             %d\n", fail_count);
    printf("Total time:         %.3f seconds\n", elapsed);
    printf("======================================\n");

    fprintf(log_file, "\n========== EXECUTION REPORT ==========\n");
    fprintf(log_file, "Total commands:     %d\n", total_commands);
    fprintf(log_file, "Successful:         %d\n", success_count);
    fprintf(log_file, "Failed:             %d\n", fail_count);
    fprintf(log_file, "Total time:         %.3f seconds\n", elapsed);
    fprintf(log_file, "======================================\n");
    fflush(log_file);

    fclose(input_file);
    fclose(log_file);
    printf("All commands executed. Output written to output.log\n");
    return 0;
}