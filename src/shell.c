#include "shell.h"

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }
    
    cmdline[pos] = '\0';
    return cmdline;
}

char** tokenize(char* cmdline) {
    // Edge case: empty command line
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // Skip leading whitespace

        if (*cp == '\0') break; // Line was only whitespace

        char* word = arglist[argnum];
        int len = 0;

        if (*cp == '<' || *cp == '>' || *cp == '|' || *cp == '&') {
            word[len++] = *cp++;             // an operator is always a token by itself
        } else {
            char quote = '\0';               // 0 = not inside quotes
            while (*cp != '\0') {
                if (quote != '\0') {
                    if (*cp == quote) {      // closing quote: drop it
                        quote = '\0';
                        cp++;
                        continue;
                    }
                } else {
                    if (*cp == '"' || *cp == '\'') {   // opening quote: drop it
                        quote = *cp;
                        cp++;
                        continue;
                    }
                    if (*cp == ' ' || *cp == '\t' || *cp == '<' ||
                        *cp == '>' || *cp == '|' || *cp == '&')
                        break;               // end of this word
                }
                if (len < ARGLEN - 1)        // never overflow the buffer
                    word[len++] = *cp;
                cp++;
            }
        }
        word[len] = '\0';
        argnum++;
    }

    if (argnum == 0) { // No arguments were parsed
        for(int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}
