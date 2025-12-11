// enseash_q1.c - Question 1: welcome message and simple prompt

#include <unistd.h>   // write
#include <string.h>   // strlen

#define WELCOME_MESSAGE "Bienvenue dans le Shell ENSEA.\nPour quitter, tapez 'exit'.\n"
#define PROMPT "enseash % "

// Small helper to write a full string to a file descriptor
static void safe_write(int fd, const char *str)
{
    size_t len = strlen(str);
    ssize_t ret = write(fd, str, len);
    (void)ret; // avoid unused warning, no error handling for now
}

int main(void)
{
    // Print welcome message
    safe_write(STDOUT_FILENO, WELCOME_MESSAGE);

    // Print first prompt
    safe_write(STDOUT_FILENO, PROMPT);

    // For Q1, we stop here: the shell just prints the message + prompt.

    return 0;
}