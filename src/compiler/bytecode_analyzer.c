#include "bytecode_analyzer.h"
#include "instruction_set.h"

int stack_effect_for(lky_instruction op, int *skip);

int calculate_max_stack_depth(unsigned char *code, int len)
{
    int max, current;
    max = current = 0;
    int i;
    for(i = 0; i < len; i++)
    {
        int skip;
        current += stack_effect_for(code[i], &skip);
        i += skip;

        max = current > max ? current : max;
    }

    return max;
}


int stack_effect_for(lky_instruction op, int *skip)
{
    *skip = 0;
    switch(op)
    {
        case LI_JUMP_FALSE:
        case LI_JUMP_TRUE:
        case LI_SAVE_MEMBER:
            *skip = 1;
        case LI_BINARY_ADD: 
        case LI_BINARY_SUBTRACT:
        case LI_BINARY_MULTIPLY:
        case LI_BINARY_DIVIDE:
        case LI_BINARY_MODULO:
        case LI_BINARY_POWER:
        case LI_BINARY_LT:
        case LI_BINARY_GT:
        case LI_BINARY_EQUAL:
        case LI_BINARY_LTE:
        case LI_BINARY_GTE:
        case LI_BINARY_NE:
        case LI_BINARY_AND:
        case LI_BINARY_OR:
        case LI_PRINT:
        case LI_POP:
        case LI_RETURN:
            return -1;
        case LI_LOAD_CONST:
        case LI_LOAD_LOCAL:
        case LI_LOAD_CLOSE:
        case LI_PUSH_NIL:
        case LI_LOAD_INDEX:
        case LI_MAKE_ARRAY:
        case LI_MAKE_TABLE:
        case LI_LOAD_MODULE:
            *skip = 1;
            return 1;
        case LI_JUMP:
        case LI_SAVE_LOCAL:
        case LI_SAVE_CLOSE:
        case LI_LOAD_MEMBER:
            *skip = 1;
        case LI_IGNORE:
        case LI_CALL_FUNC:
        case LI_MAKE_FUNCTION:
        case LI_MAKE_CLASS:
        case LI_SAVE_INDEX:
        case LI_UNARY_NOT:
        case LI_UNARY_NEGATIVE:
            return 0;
    }
    return 0;
}


