#include "shell.h"
#include <ctype.h>

// One shell variable. Variables are kept in a singly linked list.
typedef struct var {
    char*       name;
    char*       value;
    struct var* next;
} var_t;

static var_t* head = NULL;      // first variable (oldest)

// Returns the node called name, or NULL.
static var_t* find_var(const char* name) {
    for (var_t* v = head; v != NULL; v = v->next)
        if (strcmp(v->name, name) == 0)
            return v;
    return NULL;
}

// Creates the variable, or changes its value if it already exists.
static void var_set(const char* name, const char* value) {
    var_t* v = find_var(name);
    if (v != NULL) {                         // re-assignment
        free(v->value);
        v->value = strdup(value);
        return;
    }

    v = malloc(sizeof(var_t));
    v->name  = strdup(name);
    v->value = strdup(value);
    v->next  = NULL;

    if (head == NULL) {                      // append at the end, so "set"
        head = v;                            // lists variables in the order
    } else {                                 // they were created
        var_t* last = head;
        while (last->next != NULL)
            last = last->next;
        last->next = v;
    }
}

// Returns the value of the variable, or NULL if it does not exist.
const char* var_get(const char* name) {
    var_t* v = find_var(name);
    return v ? v->value : NULL;
}

// Returns 1 if word looks like NAME=value (NAME = letters, digits, '_',
// not starting with a digit).
int is_assignment(const char* word) {
    if (!(isalpha((unsigned char)word[0]) || word[0] == '_'))
        return 0;
    int i = 1;
    while (isalnum((unsigned char)word[i]) || word[i] == '_')
        i++;
    return word[i] == '=';
}

// Stores NAME=value. word must satisfy is_assignment(). It is modified.
void var_assign(char* word) {
    char* eq = strchr(word, '=');
    *eq = '\0';                              // split into NAME and value
    var_set(word, eq + 1);
}

// Built-in "set": print every variable as NAME=value.
void var_print_all(void) {
    for (var_t* v = head; v != NULL; v = v->next)
        printf("%s=%s\n", v->name, v->value);
}

/*
 * expand_variables: called after tokenizing, before executing. Every
 * argument that starts with '$' is replaced by the variable's value.
 *   - shell variables are looked up first, then environment variables
 *     (so $HOME and $USER work too)
 *   - an unknown variable expands to nothing: the argument is removed
 */
void expand_variables(char** arglist) {
    int i = 0;
    while (arglist[i] != NULL) {
        if (arglist[i][0] == '$' && arglist[i][1] != '\0') {
            const char* value = var_get(arglist[i] + 1);
            if (value == NULL)
                value = getenv(arglist[i] + 1);

            if (value == NULL) {             // unknown: drop this argument
                free(arglist[i]);
                for (int j = i; arglist[j] != NULL; j++)
                    arglist[j] = arglist[j + 1];
                continue;                    // a new word is now at position i
            }

            char* copy = strdup(value);      // new memory; main frees it later
            free(arglist[i]);
            arglist[i] = copy;
        }
        i++;
    }
}
