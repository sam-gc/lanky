#ifndef INSTRUCTION_SET_H
#define INSTRUCTION_SET_H

typedef enum {
    LI_BINARY_ADD = 50,
    LI_BINARY_SUBTRACT,
    LI_BINARY_MULTIPLY,
    LI_BINARY_DIVIDE,
    LI_BINARY_MODULO,
    LI_BINARY_POWER,
    LI_BINARY_LT,
    LI_BINARY_GT,
    LI_BINARY_EQUAL,
    LI_BINARY_LTE,
    LI_BINARY_GTE,
    LI_BINARY_NE,
    LI_BINARY_AND,
    LI_BINARY_OR,
    LI_UNARY_NOT,
    LI_UNARY_NEGATIVE,
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
    LI_RETURN,
    LI_LOAD_MEMBER,
    LI_SAVE_MEMBER,
    LI_MAKE_FUNCTION,
    LI_MAKE_CLASS,
    LI_SAVE_CLOSE,
    LI_LOAD_CLOSE,
    LI_MAKE_ARRAY,
    LI_MAKE_TABLE,
    LI_LOAD_INDEX,
    LI_SAVE_INDEX
} lky_instruction;

typedef enum {
    LT_DOUBLE,
    LT_LONG,
    LT_STRING
} lky_type;

#endif
