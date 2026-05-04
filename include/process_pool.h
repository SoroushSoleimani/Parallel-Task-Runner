#ifndef PROCESS_POOL_H
#define PROCESS_POOL_H

#include <sys/types.h>
#include <stdio.h>

typedef struct child_process {
    pid_t pid;
    int pipe_fd;
    int exit_status;
} child_process_t;

void reap_finished_processes(child_process_t *children, int *active_count, FILE *log_file, int *success_count, int *fail_count);
void wait_for_all_children(child_process_t *children, int active_count, FILE *log_file);

#endif