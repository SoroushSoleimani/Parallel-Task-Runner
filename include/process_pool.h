#ifndef PROCESS_POOL_H
#define PROCESS_POOL_H

#include <sys/types.h>
#include <stdbool.h>

typedef struct child_process child_process_t;

int create_child_process(const char *command, int pipe_write_end, pid_t *pid);
void reap_finished_processes(child_process_t *children, int *active_count, void *log_file);
void wait_for_all_children(child_process_t *children, int active_count, void *log_file);

#endif