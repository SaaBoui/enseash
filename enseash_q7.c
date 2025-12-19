// enseash_q7.c - Question 7: handle I/O redirections (< and >) + args + exit/signal + time

#include <unistd.h>     // read, write, fork, execvp, _exit, dup2
#include <string.h>     // strlen, memset, strcmp, strtok
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG
#include <stdlib.h>     // EXIT_FAILURE
#include <time.h>       // clock_gettime
#include <errno.h>
#include <fcntl.h>      // open, O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC

#define WELCOME_MESSAGE "Bienvenue dans le Shell ENSEA.\nPour quitter, tapez 'exit'.\n"
#define BYE_MESSAGE     "Bye bye...\n"

#define BUFFER_SIZE  256
#define PROMPT_SIZE  128
#define MAX_ARGS     64

static void safe_write(int fd, const char *str)
{
    (void)write(fd, str, strlen(str));
}

static void strip_eol(char *s)
{
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\n' || s[i] == '\r') {
            s[i] = '\0';
            return;
        }
    }
}

static void trim_spaces(char *s)
{
    int start = 0;
    while (s[start] == ' ' || s[start] == '\t') start++;

    if (start > 0) {
        int i = 0;
        while (s[start] != '\0') s[i++] = s[start++];
        s[i] = '\0';
    }

    int end = 0;
    while (s[end] != '\0') end++;
    while (end > 0 && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
        s[end - 1] = '\0';
        end--;
    }
}

static void append_str(char *dst, size_t *pos, size_t max, const char *src)
{
    while (*src && *pos + 1 < max) dst[(*pos)++] = *src++;
    dst[*pos] = '\0';
}

static void append_num(char *dst, size_t *pos, size_t max, unsigned long long v)
{
    char tmp[32];
    int i = 0;

    if (v == 0) tmp[i++] = '0';
    while (v > 0) { tmp[i++] = (char)('0' + (v % 10)); v /= 10; }
    while (i-- > 0 && *pos + 1 < max) dst[(*pos)++] = tmp[i];
    dst[*pos] = '\0';
}

static unsigned long long elapsed_ms(struct timespec a, struct timespec b)
{
    long sec  = b.tv_sec  - a.tv_sec;
    long nsec = b.tv_nsec - a.tv_nsec;
    if (nsec < 0) { nsec += 1000000000L; sec--; }
    return (unsigned long long)sec * 1000ULL + (unsigned long long)nsec / 1000000ULL;
}

static void build_prompt(char *prompt, size_t size, int has_last, int status, unsigned long long ms)
{
    size_t pos = 0;
    prompt[0] = '\0';

    if (!has_last) {
        append_str(prompt, &pos, size, "enseash % ");
        return;
    }

    append_str(prompt, &pos, size, "enseash [");

    if (WIFEXITED(status)) {
        append_str(prompt, &pos, size, "exit:");
        append_num(prompt, &pos, size, (unsigned long long)WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        append_str(prompt, &pos, size, "sign:");
        append_num(prompt, &pos, size, (unsigned long long)WTERMSIG(status));
    } else {
        append_str(prompt, &pos, size, "unk");
    }

    append_str(prompt, &pos, size, "|");
    append_num(prompt, &pos, size, ms);
    append_str(prompt, &pos, size, "ms] % ");
}

// Split on spaces/tabs. Modifies line in-place.
// Returns argc, and sets argv[argc] = NULL.
static int parse_args(char *line, char *argv[], int max_args)
{
    int argc = 0;
    for (char *tok = strtok(line, " \t"); tok != NULL; tok = strtok(NULL, " \t")) {
        if (argc + 1 >= max_args) break;
        argv[argc++] = tok;
    }
    argv[argc] = NULL;
    return argc;
}

// Remove 2 tokens at index i (operator + filename) by shifting left.
static void remove_two_tokens(char *argv[], int *argc, int i)
{
    for (int j = i; j + 2 <= *argc; j++) {
        argv[j] = argv[j + 2];
    }
    *argc -= 2;
    argv[*argc] = NULL;
}

// Handle redirections in child.
// Supports: cmd ... > file   and/or   cmd ... < file
// Returns 0 on success, -1 on error (and writes error).
static int setup_redirections(char *argv[], int *argc)
{
    for (int i = 0; i < *argc; ) {
        if (strcmp(argv[i], ">") == 0) {
            if (i + 1 >= *argc) {
                safe_write(STDERR_FILENO, "Error: missing filename after >\n");
                return -1;
            }
            const char *out = argv[i + 1];
            int fd = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                safe_write(STDERR_FILENO, "Error: cannot open output file\n");
                return -1;
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                close(fd);
                safe_write(STDERR_FILENO, "Error: dup2 failed\n");
                return -1;
            }
            close(fd);

            // Remove ">" and filename from argv so execvp doesn't see them
            remove_two_tokens(argv, argc, i);
            continue; // re-check same index after shift
        }

        if (strcmp(argv[i], "<") == 0) {
            if (i + 1 >= *argc) {
                safe_write(STDERR_FILENO, "Error: missing filename after <\n");
                return -1;
            }
            const char *in = argv[i + 1];
            int fd = open(in, O_RDONLY);
            if (fd < 0) {
                safe_write(STDERR_FILENO, "Error: cannot open input file\n");
                return -1;
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                close(fd);
                safe_write(STDERR_FILENO, "Error: dup2 failed\n");
                return -1;
            }
            close(fd);

            // Remove "<" and filename
            remove_two_tokens(argv, argc, i);
            continue;
        }

        i++;
    }

    return 0;
}

int main(void)
{
    char buffer[BUFFER_SIZE];
    char prompt[PROMPT_SIZE];

    int has_last = 0;
    int last_status = 0;
    unsigned long long last_ms = 0;

    safe_write(STDOUT_FILENO, WELCOME_MESSAGE);

    while (1) {
        build_prompt(prompt, sizeof(prompt), has_last, last_status, last_ms);
        safe_write(STDOUT_FILENO, prompt);

        memset(buffer, 0, sizeof(buffer));
        ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

        if (n <= 0) {
            safe_write(STDOUT_FILENO, BYE_MESSAGE);
            break;
        }

        buffer[n] = '\0';
        strip_eol(buffer);
        trim_spaces(buffer);

        if (buffer[0] == '\0') continue;

        if (strcmp(buffer, "exit") == 0) {
            safe_write(STDOUT_FILENO, BYE_MESSAGE);
            break;
        }

        char *argv[MAX_ARGS];
        int argc = parse_args(buffer, argv, MAX_ARGS);
        if (argc == 0) continue;

        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);

        pid_t pid = fork();
        if (pid < 0) {
            safe_write(STDERR_FILENO, "Error: fork failed.\n");
            continue;
        }

        if (pid == 0) {
            // Child: setup redirections then exec
            if (setup_redirections(argv, &argc) < 0) {
                _exit(EXIT_FAILURE);
            }
            if (argc == 0 || argv[0] == NULL) {
                safe_write(STDERR_FILENO, "Error: empty command\n");
                _exit(EXIT_FAILURE);
            }

            execvp(argv[0], argv);
            safe_write(STDERR_FILENO, "Error: execvp failed.\n");
            _exit(EXIT_FAILURE);
        } else {
            int w;
            do { w = waitpid(pid, &last_status, 0); } while (w == -1 && errno == EINTR);

            clock_gettime(CLOCK_MONOTONIC, &t1);

            if (w > 0) {
                last_ms = elapsed_ms(t0, t1);
                has_last = 1;
            }
        }
    }

    return 0;
}
