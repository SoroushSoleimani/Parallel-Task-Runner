#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_CMD_LEN 1024
#define MAX_LINE_LEN 4096
#define MAX_PIDS 1000

// Structure to hold information of each active child process
typedef struct {
    pid_t pid;
    int pipe_fd;      // Read end of the pipe for this child
} child_process_t;

// Write a single line to the log file with format [PID XXXX]: line
void write_log_with_pid(FILE *log_file, pid_t pid, const char *line) {
    fprintf(log_file, "[PID %d]: %s\n", pid, line);
    fflush(log_file);
}

// Read all output from a pipe and write it line by line to the log file
void read_and_log_output(FILE *log_file, int pipe_fd, pid_t pid) {
    char buffer[MAX_LINE_LEN];
    ssize_t bytes_read;
    char line_buffer[MAX_LINE_LEN];
    int line_pos = 0;

    while ((bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                line_buffer[line_pos] = '\0';
                if (line_pos > 0) {
                    write_log_with_pid(log_file, pid, line_buffer);
                }
                line_pos = 0;
            } else {
                if (line_pos < MAX_LINE_LEN - 1) {
                    line_buffer[line_pos++] = buffer[i];
                }
            }
        }
    }
    // If last line doesn't end with newline, write it anyway
    if (line_pos > 0) {
        line_buffer[line_pos] = '\0';
        write_log_with_pid(log_file, pid, line_buffer);
    }
}

// Check for finished child processes (non‑blocking with WNOHANG) and collect them
void reap_finished_processes(child_process_t *children, int *active_count, FILE *log_file) {
    int status;
    pid_t finished_pid;

    for (int i = 0; i < *active_count; i++) {
        finished_pid = waitpid(children[i].pid, &status, WNOHANG);
        if (finished_pid == children[i].pid) {
            // Child finished: read any remaining output
            read_and_log_output(log_file, children[i].pipe_fd, children[i].pid);
            close(children[i].pipe_fd);
            // Remove this child from the active array (shift left)
            for (int j = i; j < *active_count - 1; j++) {
                children[j] = children[j + 1];
            }
            (*active_count)--;
            i--;   // Re‑check the same index after shifting
        } else if (finished_pid == -1 && errno != ECHILD) {
            perror("waitpid");
        }
    }
}

// Execute a command in a child process after redirecting stdout/stderr to the pipe
void execute_command(const char *command, int pipe_write_end) {
    // Redirect stdout and stderr to the write end of the pipe
    if (dup2(pipe_write_end, STDOUT_FILENO) == -1) {
        perror("dup2 stdout");
        exit(EXIT_FAILURE);
    }
    if (dup2(pipe_write_end, STDERR_FILENO) == -1) {
        perror("dup2 stderr");
        exit(EXIT_FAILURE);
    }
    close(pipe_write_end);   // No longer needed in child because it was duplicated

    // Split the command line into arguments
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

    // Execute the command
    execvp(args[0], args);
    // If we get here, execvp failed
    fprintf(stderr, "Command not found: %s\n", args[0]);
    free(cmd_copy);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <max_concurrent> <commands_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int max_concurrent = atoi(argv[1]);
    if (max_concurrent <= 0) {
        fprintf(stderr, "Error: max_concurrent must be a positive integer.\n");
        exit(EXIT_FAILURE);
    }

    const char *input_file_path = argv[2];
    FILE *input_file = fopen(input_file_path, "r");
    if (!input_file) {
        perror("fopen input file");
        exit(EXIT_FAILURE);
    }

    FILE *log_file = fopen("output.log", "w");
    if (!log_file) {
        perror("fopen output.log");
        fclose(input_file);
        exit(EXIT_FAILURE);
    }

    child_process_t children[MAX_PIDS];
    int active_count = 0;
    char command[MAX_CMD_LEN];
    bool end_of_file = false;

    // Main loop: read commands and manage the process pool
    while (1) {
        // If we haven't reached EOF and there is room in the pool, read a new command
        if (!end_of_file && active_count < max_concurrent) {
            if (fgets(command, sizeof(command), input_file) != NULL) {
                command[strcspn(command, "\n")] = '\0';
                // Skip empty lines
                if (command[0] == '\0') {
                    continue;
                }
                // Create pipe and fork
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
                    // Child process
                    close(pipe_fds[0]);
                    execute_command(command, pipe_fds[1]);
                    exit(EXIT_FAILURE);
                } else {
                    // Parent process
                    close(pipe_fds[1]);
                    children[active_count].pid = pid;
                    children[active_count].pipe_fd = pipe_fds[0];
                    active_count++;
                }
            } else {
                // End of file reached
                end_of_file = true;
            }
        }

        // Reap any finished child processes
        reap_finished_processes(children, &active_count, log_file);

        // Exit condition: all commands processed and no active children
        if (end_of_file && active_count == 0) {
            break;
        }

        // If the pool is full and we still have commands to read, wait a bit to avoid busy looping
        if (!end_of_file && active_count == max_concurrent) {
            usleep(10000); // 10 milliseconds
        }
    }

    fclose(input_file);
    fclose(log_file);
    printf("All commands executed. Output written to output.log\n");
    return EXIT_SUCCESS;
}