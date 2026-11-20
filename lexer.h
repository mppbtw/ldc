#include <stdint.h>

enum Instruction {
    UNKNOWN = 0,
    ADD,
    SUB,
    STO,
    LDA,
    BR,
    BRZ,
    BRP,
    IN,
    OUT,
    HLT,
    DBG,
    DAT
};

enum OperandType {
    NO_OPERAND = 0,
    STR_OPERAND,
    INT_OPERAND
};

enum LineType {
    NO_MORE_LINES = 0,
    ERROR = 1,
    INSTRUCTION = 2,
};

typedef struct Line {

    int line_no;

    // LineType must be checked before the line is read
    // because it could be EOF or an error
    enum LineType ltype;

    char *line_label;
    enum Instruction inst;

    enum OperandType optype;
    char *str_operand;
    int int_operand;
} Line;

#define LINE_WORD_BUFFER_SIZE 64

typedef struct LineReader {
    char *src;
    int pos;
    int line_no;
    int src_len;
    char line_buffer[3][LINE_WORD_BUFFER_SIZE];
} LineReader;

Line read_line(LineReader *r);
LineReader new_line_reader(char *src);
