#include "assembler.h"

#include "global.h"
#include <stdbool.h>
#include <string.h>

struct LabelEntry {
    char *label;
    int definition;
};

struct UnresolvedLabel {
    char *label;
    int origin_mbox;
};

int instruction_to_code(enum Instruction i) {
    switch (i) {
        case ADD:
            return 100;
        case SUB:
            return 200;
        case STO:
            return 300;
        case LDA:
            return 500;
        case BR:
            return 600;
        case BRZ:
            return 700;
        case BRP:
            return 800;
        case IN:
            return 901;
        case OUT:
            return 902;
        case DBG:
            return 903;
        case HLT:
            return 0;
        default:
            return 0;
    }
}

MachineState assemble(char *srcbuf, int *err) {
    dbg("Beggining to assemble a file.\n");
    LineReader r = new_line_reader(srcbuf);
    *err = 0;

    struct LabelEntry labels[100];
    memset(&labels, 0, sizeof(labels));

    int u_labels_i = 0;
    struct UnresolvedLabel u_labels[100];
    memset(&u_labels, 0, sizeof(u_labels));

    MachineState m;
    memset(&m, 0, sizeof(m));

    int mbox = 0;
    Line l;
    while (1) {
        l = read_line(&r);
        if (l.ltype == ERROR) {
            errprint("lexer had an error whoopsie\n");
            *err = 1;
            return m;
        }
        if (l.ltype == NO_MORE_LINES) {
            break;
        }

        if (l.line_label != NULL) {
            // if the line has a label, check if it
            // exists already. If so there is an error
            // but if not it can be added to the labels
            for (int i=0; i<100; i++) {
                // if the space is empty, insert the new label
                if (labels[i].label == NULL) {
                    labels[i].label = l.line_label;
                    labels[i].definition = mbox; // label will point
                                                 // to the current mailbox
                    break;
                }

                if (!strcmp(labels[i].label, l.line_label)) {
                    *err = 1;
                    errprint("Label already exists\n");
                    return m;
                }
            }
        }

        int operand = l.int_operand;

        // try to resolve string operands
        // if the operand is not yet resolved, the operand will
        // be assumed for now to be 0
        // later on, u_labels will be used to fill in the operand
        // values for all unresolved labels
        if (l.optype == STR_OPERAND) {
            bool found_label = false;
            for (int i=0; i<sizeof(labels)/sizeof(labels[0]); i++) {
                if (labels[i].label == NULL)
                    break;

                if (!strcmp(labels[i].label, l.str_operand)) {
                    operand = labels[i].definition;
                    found_label = true;
                    break;
                }
            }
            if (!found_label) {
                u_labels[u_labels_i].label = l.str_operand;
                u_labels[u_labels_i].origin_mbox = mbox;
                u_labels_i++;
            }
        }

        // check that certain instructions require operands.
        // DAT is ignored since it can have an operand or not if one wishes
        if (l.inst != DAT) {
            if (l.inst == IN || l.inst == OUT || l.inst == HLT || l.inst == DBG) {
                if (l.optype != NO_OPERAND) {
                    *err = 1;
                    errprint("IN and OUT instructions cant have operands\n");
                    return m;
                }
            } else {
                if (l.optype == NO_OPERAND) {
                    *err = 1;
                    errprint("INSTRUCTION NEEDS AN OPERAND\n");
                    return m;
                }
            }
        }

        m.mailboxes[mbox] = instruction_to_code(l.inst);
        m.mailboxes[mbox] += operand;
        mbox++;
        if (mbox > 100) {
            errprint("Too many mailboxes were used.\n");
            *err = 1;
            return m;
        }
    }

    dbg("Mailboxes used: %d\n", mbox);

    
    if (verbose_enabled) {
        for (int i=0; i<100; i++) {
            if (labels[i].label == NULL)
                break;
            dbg("label: %s points to address %d\n", labels[i].label, labels[i].definition);
        }
    }

    for (int i=0; i<u_labels_i; i++) {
        for (int j=0; j<100; j++) {
            if (labels[j].label == NULL) {
                *err = 1;
                errprint("Could not resolve label: %s\n", u_labels[i].label);
                return m;
            }

            if (!strcmp(u_labels[i].label, labels[j].label)) {
                dbg("Patching unresolved label in mailbox #%d with label '%s'\n",
                        u_labels[i].origin_mbox, labels[j].label);
                m.mailboxes[u_labels[i].origin_mbox] += labels[j].definition;
                break;
            }
        }
    }

    dbg("completed the compilation with no errors\n");

    return m;
};
