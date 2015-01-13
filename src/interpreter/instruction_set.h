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
    LI_BINARY_NC,
    LI_BINARY_BAND,
    LI_BINARY_BOR,
    LI_BINARY_BXOR,
    LI_BINARY_BLSHIFT,
    LI_BINARY_BRSHIFT,
    LI_UNARY_NOT,
    LI_UNARY_NEGATIVE,
    LI_LOAD_CONST,
    LI_PRINT,
    LI_POP,
    LI_JUMP_FALSE,
    LI_JUMP_TRUE,
    LI_JUMP,
    LI_JUMP_FALSE_ELSE_POP,
    LI_JUMP_TRUE_ELSE_POP,
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
    LI_SAVE_INDEX,
    LI_SDUPLICATE,
    LI_DDUPLICATE,
    LI_FLIP_TWO,
    LI_SINK_FIRST,
    LI_MAKE_ITER,
    LI_NEXT_ITER_OR_JUMP,
    LI_LOAD_MODULE
} lky_instruction;

typedef enum {
    LT_DOUBLE,
    LT_LONG,
    LT_STRING
} lky_type;

#endif
