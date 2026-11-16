#include "shell.h"

static char* hist[HISTORY_SIZE];   // stored command lines
static int   hist_count = 0;       // how many entries are stored

// Store a copy of cmdline. If the array is full, drop the oldest entry.
void psh_hist_add(const char* cmdline) {
    if (hist_count == HISTORY_SIZE) {
        free(hist[0]);                         // forget the oldest
        for (int i = 1; i < HISTORY_SIZE; i++)
            hist[i - 1] = hist[i];             // shift everything left
        hist_count--;
    }
    hist[hist_count++] = strdup(cmdline);
}

// Print all entries, numbered from 1.
void psh_hist_print(void) {
    for (int i = 0; i < hist_count; i++)
        printf("%4d  %s\n", i + 1, hist[i]);
}

char* psh_hist_expand(const char* cmdline) {
    char* end;
    long n = strtol(cmdline + 1, &end, 10);

    if (end == cmdline + 1 || *end != '\0') {
        fprintf(stderr, "%s: usage is !n where n is a number\n", cmdline);
        return NULL;
    }
    if (n < 1 || n > hist_count) {
        fprintf(stderr, "!%ld: event not found (history has %d entries)\n", n, hist_count);
        return NULL;
    }
    return strdup(hist[n - 1]);
}
