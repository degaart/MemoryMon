#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FILENAME "/proc/meminfo"
#define PATTERN "SwapFree:"
#define THRESHOLD (512*1024)
#define INTERVAL 1

static
void notify(const char* message)
{
    int pid = fork();
    if(pid == -1) {
        fprintf(stderr, "fork() failed\n");
        abort();
    } else if(pid) {    /* parent */
        int stat;
        while(1) {
            pid_t ret = waitpid(pid, &stat, 0);
            if(WIFEXITED(stat)) {
                int retcode = WEXITSTATUS(stat);
                if(retcode != 0) {
                    fprintf(stderr, "Warning: notify-send returned with error %d\n", retcode);
                }
                break;
            } else if(WIFSIGNALED(stat)) {
                fprintf(stderr, "Warning: notify-send killed by signal %d\n", WTERMSIG(stat));
                break;
            } else if(WIFSTOPPED(stat)) {
                fprintf(stderr, "Warning: notify-send stopped by signal %d\n", WSTOPSIG(stat));
            } else if(WIFCONTINUED(stat)) {
                fprintf(stderr, "Warning: notify-send continuer\n");
            }
        }
    } else {            /* child */
        execlp("notify-send", "notify-send", message, NULL);
        perror("exec");
        abort();
    }
}

int main()
{
    size_t pat_len = strlen(PATTERN);
    int notified = 0;

    while(1) {
        FILE* f = fopen(FILENAME, "rt");
        if(!f) {
            fprintf(stderr, "Failed to open file %s\n", FILENAME);
            return 1;
        }

        char buffer[512];
        while(1) {
            char* ret = fgets(buffer, sizeof(buffer), f);
            if(!ret) {
                if(ferror(f)) {
                    fprintf(stderr, "I/O error reading %s", FILENAME);
                    fclose(f);
                    return 1;
                }
                break;
            }

            if(!strncmp(buffer, PATTERN, pat_len)) {
                char* p = buffer + pat_len;
                while(*p && *p == ' ') {
                    p++;
                }

                long long free_swap_kb = atoll(p);
                if(free_swap_kb < THRESHOLD) {
                    if(!notified) {
                        notify("Warning: Low swap space");
                        notified = 1;
                    }
                } else {
                    if(notified) {
                        notify("Ok: Enough swap space");
                        notified = 0;
                    }
                }
            }
        }

        fclose(f);
        sleep(INTERVAL);
    }
    return 0;
}
