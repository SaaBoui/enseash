// enseash_q2.c - Question 2: simple REPL executing commands without arguments

#include <unistd.h>     // read, write, execvp
#include <string.h>     // strlen, memset
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // waitpid
#include <stdlib.h>     // EXIT_FAILURE

#define WELCOME_MESSAGE "Bienvenue dans le Shell ENSEA.\nPour quitter, tapez 'exit'.\n"
#define PROMPT "enseash % "
#define BUFFER_SIZE 128

// Small helper to write a full string to a file descriptor
static void safe_write(int fd, const char *str)
{
    size_t len = strlen(str);
    ssize_t ret = write(fd, str, len);
    (void)ret; // ignore write errors for now
}

// Remove trailing '\n' if present and null-terminate the string
static void strip_newline(char *buffer)
{
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            return;
        }
    }
}

int main(void)
{
    char buffer[BUFFER_SIZE];

    // Print welcome message once at startup
    safe_write(STDOUT_FILENO, WELCOME_MESSAGE);

    while (1) {
        // Print prompt
        safe_write(STDOUT_FILENO, PROMPT);

        // Read a line from stdin
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);

        if (bytes_read <= 0) {
            // EOF (Ctrl+D) or error: on sort juste silencieusement
            break;
        }

        // Ensure null-termination
        buffer[bytes_read] = '\0';

        // Remove trailing newline if present
        strip_newline(buffer);

        // If the user just presses Enter, buffer will be an empty string -> do nothing
        if (buffer[0] == '\0') {
            continue;
        }

        // command without arguments
        // We prepare argv with only the program name and a NULL terminator.
        char *argv[2];
        argv[0] = buffer;
        argv[1] = NULL;

        pid_t pid = fork();

        if (pid < 0) {
            // fork failed
            const char *err = "Error: fork failed.\n";
            safe_write(STDERR_FILENO, err);
            continue;
        }

        if (pid == 0) {
            // Child process: replace code by the requested program
            execvp(argv[0], argv);

            // If execvp returns, an error occurred
            const char *err = "Error: execvp failed.\n";
            safe_write(STDERR_FILENO, err);
            _exit(EXIT_FAILURE);
        } else {
            // Parent process: wait for the child to finish
            int status;
            (void)waitpid(pid, &status, 0);

            // On n'affiche pas encore le code retour dans le prompt.
        }
    }

    return 0;
}