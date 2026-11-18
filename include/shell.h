#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 128
#define PROMPT "FCIT> "
#define HISTORY_SIZE 50   

// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist, int background);
int handle_builtin(char** arglist);
void psh_hist_add(const char* cmdline);
void psh_hist_print(void);
char* psh_hist_expand(const char* cmdline);
void add_job(pid_t pid, char** arglist);
void reap_jobs(void);
void print_jobs(void);

#endif // SHELL_H
