# Parallel Task Runner

## Overview
Parallel Task Runner is a C program designed for Linux environments that simulates a process pool to execute a batch of shell commands concurrently. The program reads commands from an input text file and executes them while strictly enforcing a user-defined limit on the maximum number of simultaneous active processes.

## Features
- **Concurrent Execution:** Runs multiple shell commands in parallel to maximize efficiency.
- **Process Pool Simulation:** Limits the number of active child processes to a specified integer (N).
- **Output Capturing:** Redirects and captures both standard output (`stdout`) and standard error (`stderr`) of every executed command.
- **Asynchronous Management:** Uses non-blocking system calls to actively manage and reap finished child processes without halting the parent program.

## Prerequisites
To compile and run this project, you need:
- A Linux-based operating system.
- GCC (GNU Compiler Collection) or any standard C compiler.

## Compilation
You can compile the source code using standard GCC commands. Open your terminal and run:
```bash
gcc -o task_runner main.c
*(Note: Replace `main.c` with the actual name of your source file if it is different).*

## Usage
The program requires two command-line arguments to run:
1. `N`: A positive integer representing the maximum number of concurrent processes.
2. `Path`: The path to a plain text file containing the shell commands.

**Command Format:**
bash
./task_runner <N> <input_file_path>

**Example:**
bash
./task_runner 4 commands.txt
This command will execute the shell scripts listed in `commands.txt`, ensuring that no more than 4 commands are running at the exact same time.

## Input File Format
The input file should be a standard text file where each line contains a single, complete shell command. 

Example (`commands.txt`):
text
ls -l
sleep 5
echo "Process started"
find / -name "*.txt"

## Technical Details
This project is built using core POSIX operating system concepts and system calls:
- `fork()`: Used by the parent to create child processes for each command.
- `execvp()`: Used within the child processes to replace the process image and execute the target shell command.
- `waitpid()`: Utilized with the `WNOHANG` flag by the parent process to asynchronously monitor and clean up finished child processes, allowing new ones to take their place in the pool.
- `pipe()` & `dup2()`: Used for Inter-Process Communication (IPC). The parent creates a pipe before forking. The child then uses `dup2()` to redirect its file descriptors 1 (`stdout`) and 2 (`stderr`) into the write-end of the pipe. The parent reads from the read-end to collect and format the outputs into a final output file.

## License
This project was developed as an Operating Systems university project. Feel free to use and modify it for educational purposes.
