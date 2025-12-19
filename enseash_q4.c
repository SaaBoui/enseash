// enseash_q4.c - Question 4: display exit code or signal in prompt (using execlp)

#include <unistd.h>     // read, write, execlp, _exit
#include <string.h>     // strlen, memset, strcmp
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG
#include <stdlib.h>     // EXIT_FAILURE

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
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            return;
        }
    }
}

static void buf_append_str(char *dst, size_t dst_sz, size_t *pos, const char *src)
{
    while (*src != '\0' && *pos + 1 < dst_sz) {
        dst[*pos] = *src;
        (*pos)++;
        src++;
    }
    dst[*pos] = '\0';
}

static void buf_append_u64(char *dst, size_t dst_sz, size_t *pos, unsigned long long v)
{
    char tmp[32];
    int i = 0;

    if (v == 0) {
        dst[(*pos)++] = '0';
        dst[*pos] = '\0';
        return;
    }

    while (v > 0 && i < (int)sizeof(tmp)) {
        tmp[i++] = (char)('0' + (v % 10ULL));
        v /= 10ULL;
    }

    while (i > 0 && *pos + 1 < dst_sz) {
        dst[(*pos)++] = tmp[--i];
    }
    dst[*pos] = '\0';
}

static void build_prompt(char *prompt, size_t sz, int has_last, int last_status)
{
    size_t pos = 0;
    prompt[0] = '\0';

    if (!has_last) {
        buf_append_str(prompt, sz, &pos, "enseash % ");
        return;
    }

    buf_append_str(prompt, sz, &pos, "enseash [");

    if (WIFEXITED(last_status)) {
        buf_append_str(prompt, sz, &pos, "exit:");
        buf_append_u64(prompt, sz, &pos,
                       (unsigned long long)WEXITSTATUS(last_status));
    } else if (WIFSIGNALED(last_status)) {
        buf_append_str(prompt, sz, &pos, "sign:");
        buf_append_u64(prompt, sz, &pos,
                       (unsigned long long)WTERMSIG(last_status));
    } else {
        buf_append_str(prompt, sz, &pos, "unk");
    }

    buf_append_str(prompt, sz, &pos, "] % ");
}

int main(void)
{
    char buffer[BUFFER_SIZE];
    char prompt[PROMPT_SIZE];

    int has_last = 0;
    int last_status = 0;

    safe_write(STDOUT_FILENO, WELCOME_MESSAGE);

    while (1) {
        build_prompt(prompt, sizeof(prompt), has_last, last_status);
        safe_write(STDOUT_FILENO, prompt);

        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);

        if (bytes_read <= 0) {
            safe_write(STDOUT_FILENO, BYE_MESSAGE);
            break;
        }

        buffer[bytes_read] = '\0';
        strip_newline(buffer);

        if (buffer[0] == '\0') continue;

        if (strcmp(buffer, "exit") == 0) {
            safe_write(STDOUT_FILENO, BYE_MESSAGE);
            break;
        }

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
            (void)waitpid(pid, &last_status, 0);
            has_last = 1;
        }
    }

    return 0;
}
