// enseash_q5.c - Question 5: display exit/signal and execution time in prompt (using execlp)

#include <unistd.h>     // read, write, execlp, _exit
#include <string.h>     // strlen, memset, strcmp
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG
#include <stdlib.h>     // EXIT_FAILURE
#include <time.h>       // clock_gettime
#include <errno.h>

#define WELCOME_MESSAGE "Bienvenue dans le Shell ENSEA.\nPour quitter, tapez 'exit'.\n"
#define BYE_MESSAGE     "Bye bye...\n"
#define BUFFER_SIZE     128
#define PROMPT_SIZE     128

static void safe_write(int fd, const char *str)
{
    (void)write(fd, str, strlen(str));
}

static void strip_newline(char *buffer)
{
    for (int i = 0; buffer[i]; i++) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            return;
        }
    }
}

static void append_str(char *dst, size_t *pos, size_t max, const char *src)
{
    while (*src && *pos + 1 < max) {
        dst[(*pos)++] = *src++;
    }
    dst[*pos] = '\0';
}

static void append_num(char *dst, size_t *pos, size_t max, unsigned long long v)
{
    char tmp[32];
    int i = 0;

    if (v == 0) tmp[i++] = '0';
    while (v > 0) {
        tmp[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i-- > 0 && *pos + 1 < max) {
        dst[(*pos)++] = tmp[i];
    }
    dst[*pos] = '\0';
}

static unsigned long long elapsed_ms(struct timespec start, struct timespec end)
{
    long sec  = end.tv_sec  - start.tv_sec;
    long nsec = end.tv_nsec - start.tv_nsec;

    if (nsec < 0) {
        nsec += 1000000000L;
        sec--;
    }
    return (unsigned long long)sec * 1000ULL +
           (unsigned long long)nsec / 1000000ULL;
}

static void build_prompt(char *prompt, size_t size,int has_last, int status, unsigned long long ms)
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
        append_num(prompt, &pos, size, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        append_str(prompt, &pos, size, "sign:");
        append_num(prompt, &pos, size, WTERMSIG(status));
    }

    append_str(prompt, &pos, size, "|");
    append_num(prompt, &pos, size, ms);
    append_str(prompt, &pos, size, "ms] % ");
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
        build_prompt(prompt, sizeof(prompt),
                     has_last, last_status, last_ms);
        safe_write(STDOUT_FILENO, prompt);

        memset(buffer, 0, BUFFER_SIZE);
        ssize_t n = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);

        if (n <= 0) {
            safe_write(STDOUT_FILENO, BYE_MESSAGE);
            break;
        }

        buffer[n] = '\0';
        strip_newline(buffer);

        if (buffer[0] == '\0') continue;

        if (strcmp(buffer, "exit") == 0) {
            safe_write(STDOUT_FILENO, BYE_MESSAGE);
            break;
        }

        struct timespec t_start, t_end;
        clock_gettime(CLOCK_MONOTONIC, &t_start);

        pid_t pid = fork();

        if (pid < 0) {
            safe_write(STDERR_FILENO, "Error: fork failed.\n");
            continue;
        }

        if (pid == 0) {
            execlp(buffer, buffer, (char *)NULL);
            safe_write(STDERR_FILENO, "Error: execlp failed.\n");
            _exit(EXIT_FAILURE);
        } else {
            int w;
            do {
                w = waitpid(pid, &last_status, 0);
            } while (w == -1 && errno == EINTR);

            clock_gettime(CLOCK_MONOTONIC, &t_end);

            if (w > 0) {
                last_ms = elapsed_ms(t_start, t_end);
                has_last = 1;
            }
        }
    }

    return 0;
}
