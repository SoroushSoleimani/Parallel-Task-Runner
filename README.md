# Parallel Task Runner

A C program that reads a list of shell commands from a text file and executes them concurrently using a fixed-size process pool. It captures the standard output and standard error of each command and writes them to a log file with the PID of the child process that produced them.

## Features

- Reads commands line by line from an input text file.
- Executes commands in parallel while respecting a user-defined limit on the number of simultaneous child processes (process pool).
- Uses `fork()` and `execvp()` to create and run child processes.
- Redirects each child's stdout and stderr through a dedicated pipe to the parent.
- Parent process collects output asynchronously using `waitpid()` with `WNOHANG`.
- All captured output is written to `output.log`, each line prefixed with `[PID <child_pid>]:`.
- The program never exceeds the specified number of concurrent child processes.
- Graceful cleanup: waits for all remaining children before exiting.
- Error handling: exits with a non-zero status on failure.

## Project Context

This project is the first assignment of the Operating Systems course (term 4042), instructed by Dr. Khanmirza. It aims to provide hands-on experience with fundamental OS concepts: process creation (`fork`), program execution (`exec`), inter-process communication (`pipe`, `dup2`), and process lifecycle management (`waitpid`).

## Build

A Makefile is provided. Compile the program using:

```bash
make