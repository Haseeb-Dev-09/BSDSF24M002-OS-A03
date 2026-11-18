#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

// Returns 1 if the line contains only spaces/tabs.
static int is_blank(const char* s) {
    for (; *s != '\0'; s++)
        if (*s != ' ' && *s != '\t')
            return 0;
    return 1;
}

// Runs ONE command (it contains no ';'): tokenize, then built-in or external.
static void run_command(char* segment) {
    char** arglist = tokenize(segment);
    if (arglist == NULL)                 // empty segment, e.g. "ls ;; pwd"
        return;

    // a trailing "&" means: run in the background
    int n = 0;
    while (arglist[n] != NULL)
        n++;

    int background = 0;
    if (strcmp(arglist[n - 1], "&") == 0) {
        background = 1;
        free(arglist[n - 1]);            // remove "&" from the argument list
        arglist[n - 1] = NULL;
        n--;
    }

    // after removing a trailing "&", any other "&" is misplaced
    int valid = (n > 0);
    for (int i = 0; i < n; i++)
        if (strcmp(arglist[i], "&") == 0)
            valid = 0;

    if (!valid)
        fprintf(stderr, "syntax error: '&' must come at the end of a command\n");
    else if (handle_builtin(arglist) == 0)
        execute(arglist, background);

    // Free the memory allocated by tokenize()
    for (int i = 0; arglist[i] != NULL; i++)
        free(arglist[i]);
    free(arglist);
}

int main() {
    char* cmdline;

    while (1) {
        reap_jobs();                         // collect finished background jobs
        cmdline = readline(PROMPT);
        if (cmdline == NULL)
            break;

        // "!n" is replaced by the n-th history entry BEFORE anything else
        if (cmdline[0] == '!') {
            char* expanded = psh_hist_expand(cmdline);
            free(cmdline);
            if (expanded == NULL)            // error already printed
                continue;
            cmdline = expanded;
            printf("%s\n", cmdline);         // show what is being re-run
        }

        // the WHOLE line is stored in history once, before it is split
        if (!is_blank(cmdline)) {
            psh_hist_add(cmdline);           // our history: "history" and !n
            add_history(cmdline);            // Readline history: arrow keys
        }

        // split the line at ';' and run the pieces one after another
        char* copy = strdup(cmdline);
        char* saveptr;
        for (char* seg = strtok_r(copy, ";", &saveptr);
             seg != NULL;
             seg = strtok_r(NULL, ";", &saveptr))
            run_command(seg);

        free(copy);
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}

