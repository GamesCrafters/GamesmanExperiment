#include "core/interactive/automenu.h"

#include <ctype.h>    // tolower
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // printf, fgets,
#include <stdlib.h>   // exit, EXIT_SUCCESS
#include <string.h>   // strcspn, strncmp

#include "core/misc.h"  // SafeMalloc

const size_t kKeyLengthMax = 3;

static void FormatInput(char *input) {
    // Convert all characters to lowercase.
    for (int i = 0; input[i] != '\0'; ++i) {
        input[i] = tolower(input[i]);
    }
    // Remove the trailing newline character, if any.
    // Algorithm by Tim Čas,
    // https://stackoverflow.com/a/28462221.
    input[strcspn(input, "\r\n")] = '\0';
}

static bool StringEqual(const char *s1, const char *s2, size_t n) {
    return (strncmp(s1, s2, n) == 0);
}

void GamesmanExit(const char *key) {
    (void)key;  // Unused.
    printf("Thanks for using GAMESMAN!\n");
    exit(EXIT_SUCCESS);
}

void AutoMenu(const char *title, int num_items, const char *const *items,
              const char *const *keys, const HookFunctionPointer *hooks) {
    while (1) {
        // Print menu.
        printf("%s\n\n", title);
        for (int i = 0; i < num_items; ++i) {
            printf("%s) %s\n", keys[i], items[i]);
        }
        printf("\n(b) Go back\n");
        printf("(q) Quit");

        // Prompt for input.
        bool accepted = false;
        do {
            printf("=>");
            char *input =
                (char *)SafeMalloc((kKeyLengthMax + 1) * sizeof(char));
            if (fgets(input, kKeyLengthMax + 1, stdin) == NULL) return;
            FormatInput(input);
            if (StringEqual(input, "b", kKeyLengthMax)) return;
            if (StringEqual(input, "q", kKeyLengthMax)) GamesmanExit(input);

            for (int i = 0; i < num_items; ++i) {
                if (StringEqual(input, keys[i], kKeyLengthMax)) {
                    accepted = true;
                    hooks[i](input);
                    break;
                }
            }
            if (!accepted) printf("Invalid key. Please enter again.\n");
        } while (!accepted);
    }
}
