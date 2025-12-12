# ENSEA – System Programming  
## TP: Implementation of a Minimal Unix Shell (Q1 to Q4)

**Students:** Saad BOUITA & Soel LE CORRE 
**Course:** System Programming  
**Environment:** Linux (Ubuntu via WSL)  
**Language:** C  
**Compilation:** gcc  

---

## 1. Objective of the TP

The goal of this laboratory work is to progressively implement a simple Unix shell named **ENSEASH**, using low-level POSIX system calls.

The shell is developed incrementally through several questions (Q1 to Q4 in this document), each introducing new system programming concepts such as:
- Process creation
- Program execution
- Synchronization
- Exit status handling

Only low-level system calls are used (`read`, `write`, `fork`, `execvp`, `waitpid`).  
High-level I/O functions (e.g. `printf`) are intentionally avoided.

---

## 2. Question 1 – Minimal Shell Structure

### Objective
Implement a minimal shell that:
- Displays a welcome message at startup
- Displays a prompt
- Terminates immediately

### Implementation
- The program prints the following message at startup: 
Bienvenue dans le Shell ENSEA.
Pour quitter, tapez 'exit'.

- The prompt `enseash % ` is displayed once.
- No user input or command execution is implemented at this stage.

### Concepts Illustrated
- Basic program structure
- Writing to standard output using `write()`

### File
- `enseash_q1.c`

---

## 3. Question 2 – REPL and Command Execution

### Objective
Transform the program into an interactive shell implementing a **REPL** (Read–Eval–Print Loop) capable of:
- Reading user input
- Executing simple commands (without arguments)
- Returning to the prompt after execution

### Implementation
- A `while(1)` loop is used to repeatedly:
1. Display the prompt
2. Read input using `read()`
3. Execute the command in a child process
- Process creation is done using `fork()`
- The command is executed with `execvp()`
- The parent process waits for completion using `waitpid()`
- Empty inputs (pressing Enter) are ignored

### Limitations
- Only commands without arguments are supported
- Built-in commands are not yet handled

### Concepts Illustrated
- Process creation (`fork`)
- Program replacement (`execvp`)
- Parent–child synchronization (`waitpid`)

### File
- `enseash_q2.c`

---

## 4. Question 3 – Shell Termination

### Objective
Allow the user to exit the shell cleanly:
- By typing the built-in command `exit`
- By sending an EOF (`Ctrl+D`)

In both cases, the shell must display: 
Bye bye...

### Implementation
- The input string is compared to `"exit"` using `strcmp`
- If matched, the shell exits the REPL loop
- If `read()` returns `0` (EOF), the shell also exits
- The `exit` command is handled internally and does not create a child process

### Concepts Illustrated
- EOF detection
- Built-in command handling
- Clean program termination

### File
- `enseash_q3.c`

---

## 5. Question 4 – Exit Status and Signal Reporting

### Objective
Enhance the prompt to display information about the termination of the previously executed command:
- Exit code if the command terminated normally
- Signal number if the command was terminated by a signal

### Prompt Format
- Normal termination:
enseash [exit:0] %

- Signal termination:
enseash [sign:11] %

The first prompt after startup remains:


### Implementation
- The status returned by `waitpid()` is stored
- Macros are used to interpret the status:
  - `WIFEXITED`, `WEXITSTATUS`
  - `WIFSIGNALED`, `WTERMSIG`
- The prompt is dynamically built before each command input
- Status information is updated after each executed command

### Concepts Illustrated
- Process termination analysis
- Signal handling at the shell level
- Dynamic prompt construction

### File
- `enseash_q4.c`

---

## 6. Compilation and Execution

Example compilation and execution for Q4:

```bash
gcc enseash_q4.c -o enseash_q4
./enseash_q4