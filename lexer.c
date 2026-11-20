#include "lexer.h"
#include <stdbool.h>
#include "global.h"
#include <stdlib.h>
#include <string.h>

#define stmtcmp(stmt) \
    if (!strcmp(word, #stmt)) \
        return stmt


enum Instruction decode_instruction(char *word) {
    stmtcmp(ADD);
    stmtcmp(SUB);
    stmtcmp(STO);
    stmtcmp(LDA);
    stmtcmp(BR);
    stmtcmp(BRZ);
    stmtcmp(BRP);
    stmtcmp(IN);
    stmtcmp(OUT);
    stmtcmp(HLT);
    stmtcmp(DAT);
    stmtcmp(DBG);
    return UNKNOWN;
}

LineReader new_line_reader(char *src) {
    LineReader r;
    r.pos = 0;
    r.line_no = 0;
    r.src = src;
    r.src_len = strlen(r.src);
    return r;
}

bool is_whitespace(char ch) {
    return ch == ' ';
}

void eat_whitespace(LineReader *r) {
    while (1) {
        if (r->pos >= r->src_len) {
            return;
        }
        if (!is_whitespace(r->src[r->pos])) {
            return;
        }
        r->pos++;
    }
}

void parse_operand(Line *l, char t[64]) {
    int parse = atoi(t);
    if (parse == 0 && t[0] != '0') {
        l->optype = STR_OPERAND;
        l->str_operand = malloc(LINE_WORD_BUFFER_SIZE);
        strcpy(l->str_operand, t);
        return;
    }

    l->optype = INT_OPERAND;
    l->int_operand = parse;

}

Line read_line(LineReader *r) {
    // first, skip to where the code is



    Line l;
    memset(&l, 0, sizeof(l));
    memset(r->line_buffer, 0, sizeof(r->line_buffer));

    while (1) {
        if (r->pos >= r->src_len) {
            l.ltype = NO_MORE_LINES;
            return l;
        }
        if (r->src[r->pos] != ' ' && r->src[r->pos] != '\n' && r->src[r->pos] != '#') {
            break;
        }
        if (r->src[r->pos] == '#') {
            while (1) {
                if (r->pos >= r->src_len) {
                    l.ltype = NO_MORE_LINES;
                    return l;
                }
                if (r->src[r->pos] == '\n')
                    break;
                r->pos++;
            }
        }
        r->pos++;
    }

    // line number is 1-indexed
    l.line_no = r->line_no + 1;

    int parameter = 0;
    int word_i = 0;
    char ch;
    while (1) {
        if (r->pos >= r->src_len) {
            l.ltype = NO_MORE_LINES;
            return l;
        }
        char ch = r->src[r->pos];
        if (ch == '\n') {
            r->pos++;
            break;
        }

        if (is_whitespace(ch)) {
            parameter++;
            word_i = 0;
            eat_whitespace(r);
            if (parameter == 3 && r->src[r->pos] != '\n') {
                errprint("TOO MANY PARAMETERS!!! ERROR!!!\n");
                exit(1);
            }
            if (parameter == 3)
                parameter = 2;
        } else {
            if (word_i + 2 >= LINE_WORD_BUFFER_SIZE) {
                errprint("too big of a word!!! error!!\n");
                exit(1);
            }
            r->line_buffer[parameter][word_i] = ch;
            word_i++;
            r->pos++;
        }
    }

    // there are the following types of lines
    //
    // LABEL INSTRUCTION TARGET
    // LABEL INSTRUCTION
    // INSTRUCTION TARGET
    // INSTRUCTION
    //
    // whitespace after a word can sometimes cause parameter
    // to be overcounted, so we adjust for this where the
    // word does not actually exist
    if (parameter == 2 && r->line_buffer[2][0] == 0) {
        parameter = 1;
    }
    if (parameter == 1 && r->line_buffer[1][0] == 0) {
        parameter = 0;
    }

    if (parameter == 2) {
        // must be LABEL INSTRUCTION TARGET
        l.line_label = malloc(LINE_WORD_BUFFER_SIZE);
        strcpy(l.line_label, &r->line_buffer[0][0]);

        enum Instruction inst = decode_instruction(r->line_buffer[1]);

        if (inst == UNKNOWN) {
            errprint("expected an instruction\n");
            exit(1);
        }

        l.inst = inst;
        
        parse_operand(&l, r->line_buffer[2]);
        
        l.ltype = INSTRUCTION;
        return l;
    }

    if (parameter == 1) {
        // if there are 2 words, then either its
        // INSTRUCTION TARGET or LABEL INSTRUCTION
        // we distinguish by trying to decode the first word
        enum Instruction first_word = decode_instruction(r->line_buffer[0]);
        if (first_word == UNKNOWN) {
            // its LABEL INSTRUCTION
            l.line_label = malloc(LINE_WORD_BUFFER_SIZE);
            strcpy(l.line_label, r->line_buffer[0]);

            enum Instruction second_word = decode_instruction(r->line_buffer[1]);
            if (second_word == UNKNOWN) {
                errprint("ERROR eXPECtE d an instuncktion\n");
                exit(1);
            }

            l.ltype = INSTRUCTION;
            l.inst = second_word;

            return l;
        } else {
            // ITS INSTRUCTION TARGET
            parse_operand(&l, r->line_buffer[1]);
            l.ltype = INSTRUCTION;
            l.inst = first_word;
            return l;
        }
    }

    if (parameter == 0) {
        // must be only an instruction or a blank
        // in this case, we ignore this line and return
        // the parsed value of the next line via a
        // recursive call

        enum Instruction inst = decode_instruction(r->line_buffer[0]);

        if (inst == UNKNOWN) {
            errprint("expected an instruction\n");
            exit(1);
        }

        l.ltype = INSTRUCTION;
        l.inst = inst;
    }

    return l;
}
