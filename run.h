#ifndef ASSEMBLER_H
#include "assembler.h"
#endif

typedef struct ExtraRenderInfo {
    unsigned char last_used[256];
} ExtraRenderInfo;

char *render_machine(MachineState *m, ExtraRenderInfo i);
int step_machine(MachineState *m, int *output, int input);
