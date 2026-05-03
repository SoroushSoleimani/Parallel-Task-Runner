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
