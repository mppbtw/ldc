#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

extern bool verbose_enabled;

#define TERMINAL_YELLOW "\e[0;33m"
#define BACKGROUND_GREEN "\e[42m"
#define BACKGROUND_YELLOW "\e[43m"
#define TERMINAL_COLOR_RESET "\e[0m"
#define TERMINAL_CYAN "\e[0;96m"
#define TERMINAL_BOLD "\e[1;93m"
#define TERMINAL_CHILLY "\e[1;34m"
#define TERMINAL_RED "\e[1;91m"
#define TERMINAL_GREEN "\e[1;92m"
#define TERMINAL_BOLD_INTENSE "\e[1;97m"
#define TERMINAL_ITALIC "\e[3m"
#define TERMINAL_ULINE "\e[4m"


#define dbg(...) do { if (verbose_enabled) {\
            fprintf(stdout, "%s(verbleena)%s ", TERMINAL_YELLOW, TERMINAL_COLOR_RESET); \
            fprintf(stdout, __VA_ARGS__);} \
} while (0)

#define errprint(...) do {\
            fprintf(stderr, "%s(error)%s ", TERMINAL_RED, TERMINAL_COLOR_RESET); \
            fprintf(stderr, __VA_ARGS__); \
} while (0)

#define hint(...) do {\
            fprintf(stdout, "%s(hint)%s ", TERMINAL_GREEN, TERMINAL_COLOR_RESET); \
            fprintf(stdout, __VA_ARGS__); \
} while (0)

#define dbinfo(...) do { if (verbose_enabled) {\
            fprintf(stdout, "%s(deblinfo)%s ", TERMINAL_GREEN, TERMINAL_COLOR_RESET); \
            fprintf(stdout, __VA_ARGS__);} \
} while (0)
