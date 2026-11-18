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
// Returns its exit status (0 = success, non-zero = failure).
static int run_command(char* segment) {
    char** arglist = tokenize(segment);
    if (arglist == NULL)                 // empty segment, e.g. "ls ;; pwd"
        return 0;

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

    int status = 0;
    if (!valid) {
        fprintf(stderr, "syntax error: '&' must come at the end of a command\n");
        status = 1;
    } else {
        expand_variables(arglist);           // replace $NAME by its value
        if (arglist[0] != NULL && handle_builtin(arglist) == 0)
            status = execute(arglist, background);
    }

    // Free the memory allocated by tokenize()
    for (int i = 0; arglist[i] != NULL; i++)
        free(arglist[i]);
    free(arglist);
    return status;
}

// Runs a whole line: split at ';' and run the pieces one after another.
static void run_line(const char* line) {
    char* copy = strdup(line);
    char* saveptr;
    for (char* seg = strtok_r(copy, ";", &saveptr);
         seg != NULL;
         seg = strtok_r(NULL, ";", &saveptr))
        run_command(seg);
    free(copy);
}

// Returns a pointer to the first non-blank character of s and removes
// trailing blanks (modifies s).
static char* trim(char* s) {
    while (*s == ' ' || *s == '\t')
        s++;
    char* end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t'))
        *--end = '\0';
    return s;
}

// Returns 1 if the line starts with the word "if".
static int is_if_line(const char* s) {
    while (*s == ' ' || *s == '\t')
        s++;
    return strncmp(s, "if", 2) == 0 && (s[2] == ' ' || s[2] == '\t' || s[2] == '\0');
}

#define MAX_BLOCK 20

/*
 * handle_if: first_line is the "if ..." line. Keeps reading lines (with a
 * "> " prompt) until "fi", stores the then-part and else-part, then runs the
 * condition and executes the part that matches its exit status.
 *
 *   if <command>
 *   then
 *       <commands>
 *   else            (optional)
 *       <commands>
 *   fi
 */
static void handle_if(const char* first_line) {
    char* copy = strdup(first_line);
    char* cond = trim(copy) + 2;         // skip the word "if"
    cond = trim(cond);

    if (*cond == '\0') {
        fprintf(stderr, "syntax error: 'if' needs a condition command\n");
        free(copy);
        return;
    }

    char* then_lines[MAX_BLOCK];
    char* else_lines[MAX_BLOCK];
    int then_n = 0, else_n = 0;
    enum { WANT_THEN, IN_THEN, IN_ELSE } state = WANT_THEN;
    int ok = 1;                          // becomes 0 on a syntax error

    // ---- read lines until "fi" ----
    for (;;) {
        char* raw = readline("> ");
        if (raw == NULL) {               // Ctrl+D before "fi"
            fprintf(stderr, "syntax error: missing 'fi'\n");
            ok = 0;
            break;
        }
        char* line = trim(raw);

        if (*line == '\0') {
            // blank line: ignore
        } else if (strcmp(line, "fi") == 0) {
            if (state == WANT_THEN) {
                fprintf(stderr, "syntax error: 'fi' without 'then'\n");
                ok = 0;
            }
            free(raw);
            break;
        } else if (strcmp(line, "then") == 0) {
            if (state != WANT_THEN) {
                fprintf(stderr, "syntax error: unexpected 'then'\n");
                ok = 0;
            }
            state = IN_THEN;
        } else if (strcmp(line, "else") == 0) {
            if (state != IN_THEN) {
                fprintf(stderr, "syntax error: unexpected 'else'\n");
                ok = 0;
            }
            state = IN_ELSE;
        } else if (state == WANT_THEN) {
            fprintf(stderr, "syntax error: expected 'then'\n");
            ok = 0;
        } else if (state == IN_THEN && then_n < MAX_BLOCK) {
            then_lines[then_n++] = strdup(line);
        } else if (state == IN_ELSE && else_n < MAX_BLOCK) {
            else_lines[else_n++] = strdup(line);
        } else {
            fprintf(stderr, "syntax error: too many commands in block\n");
            ok = 0;
        }
        free(raw);
    }

    // ---- run the condition, then the matching block ----
    if (ok) {
        int status = run_command(cond);  // 0 = success -> then, else -> else
        if (status == 0) {
            for (int i = 0; i < then_n; i++)
                run_line(then_lines[i]);
        } else {
            for (int i = 0; i < else_n; i++)
                run_line(else_lines[i]);
        }
    }

    for (int i = 0; i < then_n; i++) free(then_lines[i]);
    for (int i = 0; i < else_n; i++) free(else_lines[i]);
    free(copy);
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

        if (is_if_line(cmdline))
            handle_if(cmdline);          // reads the rest of the if-block itself
        else
            run_line(cmdline);           // split at ';' and run each piece

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}

