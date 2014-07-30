#ifndef INSTRUCTION_SET_H
#define INSTRUCTION_SET_H

typedef enum {
    LI_BINARY_ADD = 100,
    LI_BINARY_SUBTRACT,
    LI_BINARY_MULTIPLY,
    LI_BINARY_DIVIDE,
    LI_BINARY_MODULO,
    LI_BINARY_LT,
    LI_BINARY_GT,
    LI_BINARY_EQUAL,
    LI_BINARY_LTE,
    LI_BINARY_GTE,
    LI_BINARY_NE,
    LI_LOAD_CONST,
    LI_PRINT,
    LI_POP,
    LI_JUMP_FALSE,
    LI_JUMP_TRUE,
    LI_JUMP,
    LI_IGNORE,
    LI_SAVE_LOCAL,
    LI_LOAD_LOCAL,
    LI_PUSH_NIL,
    LI_CALL_FUNC,
    LI_RETURN
} lky_instruction;

typedef enum {
    LT_DOUBLE,
    LT_LONG,
    LT_STRING
} lky_type;

#endif