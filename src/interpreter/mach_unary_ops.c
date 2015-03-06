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

#include "mach_unary_ops.h"

#define IS_TAGGED(a) ((uintptr_t)(a) & 1)
#define CHECK_EXEC_CUSTOM_IMPL(a, name) \
    do { \
        if(!(IS_TAGGED(a)) && a->type == LBI_CUSTOM || !(IS_TAGGED(a)) && a->type == LBI_CUSTOM_EX) {\
            lky_object *func = lobj_get_member(a, name); \
            if(!func || func->type != LBI_FUNCTION) \
                break;\
            lky_object_function *cfunc = (lky_object_function *)func;\
            if(cfunc->callable.argc && cfunc->callable.argc != 2) \
                break;\
            return un_op_exec_custom(cfunc); \
        } \
    } while(0)

lky_object *un_op_exec_custom(lky_object_function *func)
{
    lky_callable c = func->callable;

    //return (lky_object *)c.function(MAKE_BUNDLE(func, NULL));
    lky_func_bundle b = MAKE_BUNDLE(func, NULL);
    return (lky_object *)c.function(&b);
}

lky_object *lobjb_unary_not(lky_object *a)
{
    CHECK_EXEC_CUSTOM_IMPL(a, "op_not_");   
    lky_object_builtin *ac = (lky_object_builtin *)a;

    if(IS_TAGGED(a))
        return LKY_TESTC_FAST(!OBJ_NUM_UNWRAP(a));

    switch(a->type)
    {
        case LBI_FLOAT:
        case LBI_INTEGER:
            return LKY_TESTC_FAST(!OBJ_NUM_UNWRAP(ac));
        case LBI_BOOL:
            return a == &lky_yes ? &lky_no : &lky_yes;
        default:
            break;
    }

    return LKY_TESTC_FAST(a == &lky_nil);
}
