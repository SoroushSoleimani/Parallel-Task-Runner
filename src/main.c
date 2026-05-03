#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "process_pool.h"
#include "log_manager.h"

#define MAX_CMD_LEN 1024
#define MAX_PIDS 1000

void execute_command(const char *command, int pipe_write_end) {
    if (dup2(pipe_write_end, STDOUT_FILENO) == -1) {
        perror("dup2 stdout");
        exit(EXIT_FAILURE);
    }
    if (dup2(pipe_write_end, STDERR_FILENO) == -1) {
        perror("dup2 stderr");
        exit(EXIT_FAILURE);
    }
    close(pipe_write_end);

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

    child_process_t children[MAX_PIDS];
    int active_count = 0;
    char command[MAX_CMD_LEN];
    bool end_of_file = false;

    while (1) {
        if (!end_of_file && active_count < max_concurrent) {
            if (fgets(command, sizeof(command), input_file) != NULL) {
                command[strcspn(command, "\n")] = '\0';
                if (command[0] == '\0') continue;

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
                    close(pipe_fds[0]);
                    execute_command(command, pipe_fds[1]);
                    exit(EXIT_FAILURE);
                } else {
                    close(pipe_fds[1]);
                    children[active_count].pid = pid;
                    children[active_count].pipe_fd = pipe_fds[0];
                    active_count++;
                }
            } else {
                end_of_file = true;
            }
        }
        reap_finished_processes(children, &active_count, log_file);
        if (end_of_file && active_count == 0) break;
        if (!end_of_file && active_count == max_concurrent) {
            usleep(10000);
        }
    }

    fclose(input_file);
    fclose(log_file);
    printf("All commands executed. Output written to output.log\n");
    return 0;
}