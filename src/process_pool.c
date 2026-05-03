#include "process_pool.h"
#include "log_manager.h"
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

int create_child_process(const char *command, int pipe_write_end, pid_t *pid) {
    pid_t p = fork();
    if (p == -1) {
        perror("fork");
        return -1;
    }
    if (p == 0) {
        // child: will execute command (called from main after dup2)
        // This function only returns in parent; child should not return here.
        return 0;
    }
    *pid = p;
    return 0;
}

void reap_finished_processes(child_process_t *children, int *active_count, FILE *log_file) {
    int status;
    pid_t finished_pid;
    for (int i = 0; i < *active_count; i++) {
        finished_pid = waitpid(children[i].pid, &status, WNOHANG);
        if (finished_pid == children[i].pid) {
            read_and_log_output(log_file, children[i].pipe_fd, children[i].pid);
            close(children[i].pipe_fd);
            for (int j = i; j < *active_count - 1; j++) {
                children[j] = children[j + 1];
            }
            (*active_count)--;
            i--;
        } else if (finished_pid == -1 && errno != ECHILD) {
            perror("waitpid");
        }
    }
}

void wait_for_all_children(child_process_t *children, int active_count, FILE *log_file) {
    for (int i = 0; i < active_count; i++) {
        waitpid(children[i].pid, NULL, 0);
        read_and_log_output(log_file, children[i].pipe_fd, children[i].pid);
        close(children[i].pipe_fd);
    }
}