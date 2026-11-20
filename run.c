#include "global.h"
#include <stdbool.h>
#include <string.h>
#include "run.h"

#define GRIDROW  "---------------------------------------------------------------------------------\n"

#define RENDER_STRING_SIZE 2048

#define STR_APPEND(s) for (int i=0; i<strlen(s); i++) { \
    str[str_i] = s[i]; \
    str_i++; \
}

// return values:
// 0 default
// 1 if the machine outputted,
// 2 if the machine requests an input
// 3 if there was an error
// 4 if the program has halted
// 5 if the program wants to output its debug screen
int step_machine(MachineState *m, int *output, int input) {
    //dbg("running instruction in box %d: %d\n", m->pc, m->mailboxes[m->pc]);
    if (m->waiting_for_input) {
        m->calculator = input;
        m->waiting_for_input = false;
    }

    int instruction = m->mailboxes[m->pc];
    int opcode = instruction / 100;
    int operand = instruction - opcode*100;
    switch (opcode) {
        case 0: // HLT
            return 4;
            break;
        case 1: // ADD
            m->calculator += m->mailboxes[operand];
            break;
        case 2: // SUB
            m->calculator -= m->mailboxes[operand];
            break;
        case 3: // STO
            m->mailboxes[operand] = m->calculator;
            break;
        case 5: // LDA
            m->calculator = m->mailboxes[operand];
            break;
        case 6: // BR
            m->pc = operand;
            return 0;
        case 7: // BRZ
            if (m->calculator == 0) {
                m->pc = operand;
                return 0;
            }
            break;
        case 8: // BRP
            if (m->calculator >= 0) {
                m->pc = operand;
                return 0;
            }
            break;
        default:
            if (instruction == 901) {
                m->waiting_for_input = true;
                m->pc++;
                return 2;
            } else if (instruction == 902) {
                *output = m->calculator;
                m->pc++;
                return 1;
            } else if (instruction == 903) {
                m->pc++;
                return 5;
            }
            break;
    }
    m->pc++;
    return 0;
}

int add_number_row(char *str, int str_i, int start_index) {
    STR_APPEND("-");
    for (int i=0; i<20; i++) {
        STR_APPEND("");
        STR_APPEND(TERMINAL_CHILLY);
        sprintf(str+str_i, "%02d", start_index + i);
        str_i += 2;
        STR_APPEND(TERMINAL_COLOR_RESET);
        STR_APPEND("--")
    }
    return str_i;
}

char *render_machine(MachineState *m, ExtraRenderInfo info) {
    char *str = malloc(RENDER_STRING_SIZE);
    int str_i = 0;
    memset(str, 0, RENDER_STRING_SIZE);

    str_i = add_number_row(str, str_i, 0);
    STR_APPEND("\n");
    for (int i=0; i<100; i++) {
        STR_APPEND("|");
        if (info.last_used[i] < 10) {
            STR_APPEND(BACKGROUND_GREEN);
        }
        sprintf(str+str_i, "%03d", m->mailboxes[i]);
        str_i += 3;
        STR_APPEND(TERMINAL_COLOR_RESET);
        if ((i+1) % 20 == 0) {
            STR_APPEND("|\n");
            STR_APPEND("-");
            for (int i=0; i<strlen(GRIDROW)/3; i++) {
                STR_APPEND("-0-");
            }
            STR_APPEND("\n");
            //STR_APPEND(GRIDROW);
        }
    }
    return str;
};
