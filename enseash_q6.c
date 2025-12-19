// enseash_q6.c - Question 6: support arguments (argv parsing) + exit/signal + time in prompt

#include <unistd.h>     // read, write, fork, execvp, _exit
#include <string.h>     // strlen, memset, strcmp, strtok
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG
#include <stdlib.h>     // EXIT_FAILURE
#include <time.h>       // clock_gettime
#include <errno.h>

#define WELCOME_MESSAGE "Bienvenue dans le Shell ENSEA.\nPour quitter, tapez 'exit'.\n"
#define BYE_MESSAGE     "Bye bye...\n"

#define BUFFER_SIZE  256
#define PROMPT_SIZE  128
#define MAX_ARGS     32   // max number of tokens (command + args)

static void safe_write(int fd, const char *str)
{
    (void)write(fd, str, strlen(str));
}

static void strip_eol(char *s)
{
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\n' || s[i] == '\r') { // handles Windows CRLF too
            s[i] = '\0';
            return;
        }
    }
}

static void trim_spaces(char *s)
{
    // trim leading spaces/tabs
    int start = 0;
    while (s[start] == ' ' || s[start] == '\t') start++;

    if (start > 0) {
        int i = 0;
        while (s[start] != '\0') s[i++] = s[start++];
        s[i] = '\0';
    }

    // trim trailing spaces/tabs
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

// Tokenize the input line into argv[] (whitespace split).
// Returns argc. argv[argc] is set to NULL.
static int parse_args(char *line, char *argv[], int max_args)
{
    int argc = 0;

    // strtok modifies line in-place
    for (char *tok = strtok(line, " \t"); tok != NULL; tok = strtok(NULL, " \t")) {
        if (argc + 1 >= max_args) break; // keep space for NULL terminator
        argv[argc++] = tok;
    }
    argv[argc] = NULL;
    return argc;
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

        // Q6: build argv from the command line
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
