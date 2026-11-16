#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        // "!n" is replaced by the n-th history entry BEFORE tokenizing
        if (cmdline[0] == '!') {
            char* expanded = psh_hist_expand(cmdline);
            free(cmdline);
            if (expanded == NULL)        // error already printed
                continue;
            cmdline = expanded;
            printf("%s\n", cmdline);     // show what is being re-run
        }

        if ((arglist = tokenize(cmdline)) != NULL) {
            psh_hist_add(cmdline);       // only non-empty commands are stored

            if (handle_builtin(arglist) == 0)
                execute(arglist);

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
