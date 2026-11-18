#include "shell.h"

#define MAX_JOBS 32

typedef struct {
    pid_t pid;
    char  cmd[MAX_LEN];
} job_t;

static job_t jobs[MAX_JOBS];     // table of running background jobs
static int   job_count = 0;

// Remember a new background job (pid + the command text, for "jobs").
void add_job(pid_t pid, char** arglist) {
    if (job_count == MAX_JOBS) {
        fprintf(stderr, "too many background jobs, not tracking pid %d\n", pid);
        return;
    }
    jobs[job_count].pid = pid;
    jobs[job_count].cmd[0] = '\0';
    for (int i = 0; arglist[i] != NULL; i++) {
        strncat(jobs[job_count].cmd, arglist[i], MAX_LEN - strlen(jobs[job_count].cmd) - 2);
        strncat(jobs[job_count].cmd, " ", MAX_LEN - strlen(jobs[job_count].cmd) - 1);
    }
    job_count++;
}

// Collect every finished background child WITHOUT blocking, and drop it
// from the table. Called before each prompt, so no zombies pile up.
void reap_jobs(void) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].pid == pid) {
                printf("[done] pid %d  %s\n", pid, jobs[i].cmd);
                for (int j = i + 1; j < job_count; j++)   // close the gap
                    jobs[j - 1] = jobs[j];
                job_count--;
                break;
            }
        }
    }
}

// Built-in "jobs": list the background jobs that are still running.
void print_jobs(void) {
    if (job_count == 0) {
        printf("No background jobs.\n");
        return;
    }
    for (int i = 0; i < job_count; i++)
        printf("[%d] pid %d  Running  %s\n", i + 1, jobs[i].pid, jobs[i].cmd);
}
