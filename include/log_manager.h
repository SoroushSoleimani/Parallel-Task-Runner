#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <stdio.h>
#include <sys/types.h>

FILE* open_log_file(const char *filename);
void write_log_with_pid(FILE *log_file, pid_t pid, const char *line);
void read_and_log_output(FILE *log_file, int pipe_fd, pid_t pid);

#endif