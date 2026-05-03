#include "log_manager.h"
#include <unistd.h>
#include <string.h>

#define MAX_LINE_LEN 4096

FILE* open_log_file(const char *filename) {
    return fopen(filename, "w");
}

void write_log_with_pid(FILE *log_file, pid_t pid, const char *line) {
    fprintf(log_file, "[PID %d]: %s\n", pid, line);
    fflush(log_file);
}

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
    if (line_pos > 0) {
        line_buffer[line_pos] = '\0';
        write_log_with_pid(log_file, pid, line_buffer);
    }
}