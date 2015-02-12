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

#ifndef AST_MACHINE_H
#define AST_MACHINE_H

#include "lkyobj_builtin.h"
#include "arraylist.h"

struct interp;

typedef struct stackframe {
    struct stackframe *next;
    struct stackframe *prev;
    
    arraylist parent_stack;
    lky_object *bucket;
    void **constants;
    void **locals;
    void **data_stack;
    int *catch_stack;
    char **names;
    long pc;
    unsigned char *ops;
    long tape_len;
    
    struct interp *interp;
    
    long stack_pointer;
    long stack_size;
    long locals_count;
    
    int catch_pointer;
    lky_object *ret;
    lky_object *thrown;
} stackframe;

typedef struct interp {
    stackframe *stack;
    hashtable stdlib;
} mach_interp;

typedef struct lky_object_function lky_object_function;

mach_interp mach_make_interp();

lky_object *mach_interrupt_exec(lky_object_function *func);
lky_object *mach_execute(lky_object_function *func);
void mach_halt_with_err(lky_object *err);

#endif
