# Custom POSIX Shell Implementation

## Overview
This project is a **POSIX-compliant interactive Unix shell** implemented in **C**, developed as part of the **Advanced Operating Systems** course.  
The shell replicates core functionalities of standard Unix shells such as **bash**, demonstrating deep understanding of **process management, inter-process communication, file descriptors, and signal handling**.

The implementation avoids prohibited system calls (`system`, `popen`) and relies strictly on **low-level Unix primitives** like `fork`, `execvp`, `pipe`, `dup2`, and `waitpid`.

---

## Key Features

### 1. Interactive Shell Prompt
- Displays prompt in the format:  
  ```
  <username@system_name:current_directory>
  ```
- Supports `~` for home directory
- Handles arbitrary spaces and tabs

---

### 2. Built-in Commands
Implemented internally without using `execvp`:

| Command | Description |
|------|-----------|
| `cd` | Directory navigation (`.`, `..`, `~`, `-`) |
| `pwd` | Prints current working directory |
| `echo` | Prints input arguments |
| `ls` | Directory listing with `-a`, `-l`, and combined flags |
| `pinfo` | Displays process info using `/proc` |
| `search` | Recursive search for files/directories |
| `history` | Persistent command history across sessions |

---

### 3. System Command Execution
- Uses `fork()` + `execvp()` to execute external programs
- Supports arguments and multiple commands
- Proper error handling using `perror`

---

### 4. Foreground & Background Processes
- Commands ending with `&` run in background
- Prints PID of background processes
- Supports multiple concurrent background jobs
- Prevents zombie processes using `waitpid`

---

### 5. Input / Output Redirection
Supports:
- Input redirection: `<`
- Output redirection (overwrite): `>`
- Output redirection (append): `>>`

Example:
```bash
sort < input.txt > output.txt
```

Internally implemented using `open()` and `dup2()`.

---

### 6. Command Pipelines
- Supports arbitrary-length pipelines using `|`
- Correct file descriptor management to avoid deadlocks
- Compatible with redirection inside pipelines

Example:
```bash
cat file.txt | grep error | wc -l
```

---

### 7. Signal Handling & Job Control
- `Ctrl+C` (SIGINT): Terminates foreground process
- `Ctrl+Z` (SIGTSTP): Stops foreground process
- `Ctrl+D`: Exits shell cleanly
- Shell process remains unaffected by signals sent to child processes

---

### 8. Autocomplete (TAB)
- Lists matching files/directories in current directory
- Auto-completes if a single match exists

---

### 9. Persistent Command History
- Stores up to 20 commands
- Displays last 10 by default
- Survives across shell restarts
- Supports `history <n>` and UP arrow navigation

---

## Project Structure
```
.
├── README.md
├── Makefile
├── source files (*.c, *.h)
```

---

## Compilation & Execution

### Compile
```bash
make
```

### Run
```bash
./a.out
```

---

## Design Highlights
- Process isolation using `fork()` + `execvp()`
- Explicit file descriptor control via `dup2()`
- Deadlock-free pipeline execution
- Robust signal handling
- Modular design

---

## Known Limitations
- No environment variable expansion
- No advanced job control (`fg`, `bg`)
- No quote or escape handling
- Autocomplete limited to current directory

---

## Author
**Abhradeep Das**
