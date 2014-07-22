#ifndef INSTRUCTION_SET_H
#define INSTRUCTION_SET_H

typedef enum {
    LI_BINARY_ADD = 100,
    LI_BINARY_SUBTRACT,
    LI_BINARY_MULTIPLY,
    LI_BINARY_DIVIDE,
    LI_LOAD_CONST,
    LI_PRINT,
    LI_POP,
    LI_JUMP_FALSE,
    LI_JUMP_TRUE
} lky_instruction;

typedef enum {
    LT_DOUBLE,
    LT_LONG,
    LT_STRING
} lky_type;

#endif