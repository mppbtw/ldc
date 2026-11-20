
#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "lexer.h"
#include <stdbool.h>

typedef struct MachineState {
    int mailboxes[100];
    int pc;
    int16_t calculator;
    bool waiting_for_input;
    bool is_neg;
} MachineState;

MachineState assemble(char *srcbuf, int *err);

#endif
