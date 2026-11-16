#include "shell.h"

int execute(char* arglist[]) {
    int status;
    int cpid = fork();

    switch (cpid) {
        case -1:
            perror("fork failed");
            exit(1);
        case 0: // Child process
            execvp(arglist[0], arglist);
            perror("Command not found"); // This line runs only if execvp fails
            exit(1);
        default: // Parent process
            waitpid(cpid, &status, 0);
            // printf("Child pid:%d exited with status %d\n", cpid, status >> 8);
            return 0;
    }
}

int handle_builtin(char** arglist) {
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Shell exited.\n");
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        const char* dir = arglist[1];
        if (dir == NULL)                 // "cd" alone goes to HOME
            dir = getenv("HOME");
        if (dir == NULL || chdir(dir) != 0)
            fprintf(stderr, "cd: %s: %s\n", dir ? dir : "(no HOME)", strerror(errno));
        return 1;
    }
    
    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <dir>   change the current directory\n");
        printf("  exit       exit the shell\n");
        printf("  help       show this help message\n");
        printf("  jobs       list background jobs (not yet implemented)\n");
        printf("  history    show previously entered commands\n");
        return 1;
    }
    
    if (strcmp(arglist[0], "history") == 0) {
        psh_hist_print();
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    return 0;   // not a built-in
}
