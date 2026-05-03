# Concurrent Task Executor

## Overview

The **Concurrent Task Executor** is a robust, POSIX-compliant C program that executes a batch of shell commands in parallel while strictly controlling system resources. It implements a custom process pool architecture: commands are read from a text file and dispatched concurrently, ensuring that the number of active child processes never exceeds a user‑defined limit `N`.

This project demonstrates core operating system concepts:
- Process lifecycle management (`fork`, `execvp`, `waitpid`)
- Inter‑process communication (IPC) using anonymous pipes
- I/O redirection (`dup2`)
- Non‑blocking asynchronous monitoring (`WNOHANG`)

## Core Features

- **Bounded Concurrency** – Prevents resource exhaustion by enforcing a maximum number of simultaneous child processes.
- **Asynchronous Reaping** – Uses non‑blocking `waitpid()` with `WNOHANG` to reap zombie processes immediately, keeping the scheduler responsive.
- **IPC & I/O Redirection** – Captures `stdout` and `stderr` from each command using anonymous pipes and file descriptor duplication.
- **Deterministic Output Tracing** – Formats captured output by prefixing every line with the originating Process ID (PID), making concurrent logs readable and traceable.

## System Architecture

The application follows a **producer‑consumer model** managed by a single parent scheduler:

1. **Process Dispatching (`fork` & `execvp`)**  
   The parent reads commands from the input file line by line. If the current number of active children is below `N`, it forks a new child. The child then calls `execvp()` to replace its memory space with the target shell command.

2. **Stream Redirection (`pipe` & `dup2`)**  
   Before the `fork`, the parent creates a pipe for each command. After forking, the child closes the read end of the pipe and duplicates the write end to both `stdout` (FD 1) and `stderr` (FD 2). This redirects all output from the command back to the parent through the pipe.

3. **Non‑Blocking Monitoring (`waitpid` with `WNOHANG`)**  
   The parent continuously polls the process pool using `waitpid()` with the `WNOHANG` flag. This allows it to reap finished children immediately, free pool slots, and dispatch pending commands without blocking. The parent never calls `wait()` (which would block).

4. **Log Aggregation**  
   For each active child, the parent reads from the read end of its associated pipe. Every line of raw output is prefixed with `[PID: <child_pid>] ` and then written to the final output (typically `stdout`). This ensures that interleaved outputs are still traceable.

## Getting Started

### Prerequisites

- Linux or any POSIX‑compliant operating system (macOS, WSL, BSD, etc.)
- GCC (GNU Compiler Collection) or any C11-compliant compiler

### Compilation

Use the following command to build the program:

```bash
gcc -Wall -Wextra -O2 -o task_executor main.c
```

If you have multiple source files, adjust accordingly:
```bash
gcc -Wall -Wextra -O2 -o task_executor main.c process_pool.c -lpthread
```

## Usage
The program expects exactly two command‑line arguments:
```bash
./task_executor <max_concurrent_processes> <input_file>
```

max_concurrent_processes – a positive integer (N) that limits the number of simultaneously running child processes.

input_file – path to a text file containing shell commands, one per line.

The program will execute each line as a separate command, output the results with PID prefixes, and exit only after all commands have completed.

**Detailed Design & Code Structure**

### Data Structures
```bash
typedef struct {
    pid_t pid;
    int pipe_fd;      // read end of the pipe for this child
    char *command;    // optional: store the command for debugging
} child_process_t;
```
### Example
**1. Create an input file jobs.txt**
```bash
sleep 2
echo "Task A Complete"
ls -la /tmp
echo "Task B Complete"
pwd
```
**2. Execute with a maximum of 3 concurrent processes**
```bash
./task_executor 3 jobs.txt
```
**3. Example Output
Because commands run in parallel, the exact order of lines depends on the OS scheduler. However, every output line is strictly prefixed with its executor’s PID. The output will appear similar to:**
```bash
[PID: 4051] Task A Complete
[PID: 4052] total 12
[PID: 4052] drwxrwxrwt 14 root root 4096 May 3 12:00 .
[PID: 4052] drwxr-xr-x 20 root root 4096 May 1 09:00 ..
[PID: 4053] Task B Complete
[PID: 4054] /home/user/project
```


### Technical Constraints & Error Handling
**Command failure handling** – If execvp fails (e.g., command not found), the child writes the error message (via perror) to the pipe because stderr is also redirected. The parent will print that message prefixed with the child’s PID.

**File descriptor leaks** – The parent closes the unused write end of each pipe immediately after fork. The child closes the read end. The parent also closes each pipe’s read end after the child terminates and all output is consumed.

**Zombie prevention** – Using waitpid with WNOHANG ensures that no zombie process remains. The parent reaps children the moment they exit.

**Line buffering** – The output reading function handles partial lines and splits correctly, even if pipe reads break lines into multiple chunks.

**Resource limits** – The program respects the user‑specified N. If N is too high (e.g., 1000), the OS’s process limit (ulimit -u) will eventually prevent further fork. The program reports fork failures and continues with remaining commands.

**Signal safety** – The parent does not use signal() or sigaction(); it relies purely on non‑blocking polling. This avoids race conditions common with SIGCHLD handlers.

## Performance Considerations
**Polling vs. blocking** – The parent uses a small usleep (or select() on the pipe file descriptors) to reduce CPU usage. In a production version, you might use select() or epoll() to sleep until any pipe has data or any child exits, but the simple polling approach is sufficient for educational purposes and moderate workloads.

**Memory usage** – The program stores at most N pipe file descriptors and child PIDs. No per‑command output buffering beyond the page‑size read buffer.

**Scalability** – With N=100 and a fast input file, the scheduler remains responsive because waitpid is called immediately after each dispatch cycle.

### License
This project is open source and available under the MIT License.

### Author
Soroush Soleimani

**For questions or contributions, please open an issue or submit a pull request on GitHub.**
