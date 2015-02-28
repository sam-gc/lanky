/* Lanky -- Scripting Language and Virtual Machine
 * Copyright (C) 2014  Sam Olsen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "bytecode_analyzer.h"
#include "instruction_set.h"

int stack_effect_for(lky_instruction op, int *skip, unsigned char *code, int i);

int calculate_max_stack_depth(unsigned char *code, int len)
{
    int max, current;
    max = current = 0;
    int i;
    for(i = 0; i < len; i++)
    {
        int skip;
        current += stack_effect_for(code[i], &skip, code, i);
        i += skip;

        max = current > max ? current : max;
    }

    return max;
}

// This function is highly erroneous, but in this case if the
// value it returns is wrong, the wrong value will *always* be
// greater than the necessary depth, which will allow everything
// to run properly.
int calculate_max_catch_depth(unsigned char *code, int len)
{
    int max;
    int i;
    for(i = max = 0; i < len; i++)
        if(code[i] == LI_PUSH_CATCH)
            max++;

    return max;
}

int stack_effect_for(lky_instruction op, int *skip, unsigned char *code, int i)
{
    *skip = 0;
    switch(op)
    {
        case LI_JUMP_FALSE:
        case LI_JUMP_TRUE:
        case LI_SAVE_MEMBER:
            *skip = 4;
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
        case LI_BINARY_NC:
        case LI_BINARY_BAND:
        case LI_BINARY_BOR:
        case LI_BINARY_BXOR:
        case LI_BINARY_BLSHIFT:
        case LI_BINARY_BRSHIFT:
        case LI_PRINT:
        case LI_POP:
        case LI_RETURN:
            return -1;
        case LI_LOAD_CONST:
        case LI_LOAD_LOCAL:
        case LI_LOAD_CLOSE:
        case LI_LOAD_INDEX:
        case LI_MAKE_ARRAY:
        case LI_MAKE_TABLE:
        case LI_LOAD_MODULE:
            *skip = 1;
        case LI_SDUPLICATE:
        case LI_PUSH_NIL:
        case LI_PUSH_NEW_OBJECT:
        case LI_ITER_INDEX:
            return 1;
        case LI_NEXT_ITER_OR_JUMP:
            *skip = 4;
            return 1;
        case LI_JUMP_TRUE_ELSE_POP:
        case LI_JUMP_FALSE_ELSE_POP:
        case LI_JUMP:
        case LI_SAVE_LOCAL:
        case LI_SAVE_CLOSE:
        case LI_LOAD_MEMBER:
            *skip = 1;
        case LI_IGNORE:
        case LI_CALL_FUNC:
        case LI_MAKE_FUNCTION:
        case LI_SAVE_INDEX:
        case LI_UNARY_NOT:
        case LI_UNARY_NEGATIVE:
            return 0;
        case LI_DDUPLICATE:
            return 2;
        case LI_MAKE_CLASS:
        {
            int howmany = code[++i];
            *skip = howmany * 2;
            return howmany;
        }
    }
    return 0;
}


