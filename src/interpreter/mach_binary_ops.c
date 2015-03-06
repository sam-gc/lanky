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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mach_binary_ops.h"
#include "lky_machine.h"

#define IS_TAGGED(a) ((uintptr_t)(a) & 1)
#define IS_SINGLETON(a) (a && (a == &lky_nil || a == &lky_yes || a == &lky_no))
#define EITHER_SIGNLETONS(a, b) (IS_SINGLETON(a) || IS_SINGLETON(b))
#define IS_NIL(a) (a == &lky_nil)
#define EITHER_NIL(a, b) (IS_NIL(a) || IS_NIL(b))
#define OBJ_NUM_PROMO(a, b) ((!((uintptr_t)(a) & 1) && a->type == LBI_FLOAT) || (!((uintptr_t)(b) & 1) && b->type == LBI_FLOAT ) ? LBI_FLOAT : LBI_INTEGER) 
#define CHECK_EXEC_CUSTOM_IMPL(a, b, name) \
    do { \
        if(!(IS_TAGGED(a)) && a->type == LBI_CUSTOM || !(IS_TAGGED(a)) && a->type == LBI_CUSTOM_EX || !(IS_TAGGED(b)) && b->type == LBI_CUSTOM || !(IS_TAGGED(b)) && b->type == LBI_CUSTOM_EX) { \
            lky_object *caller = !(IS_TAGGED(a)) && (a->type == LBI_CUSTOM || a->type == LBI_CUSTOM_EX) ? a : b;\
            lky_object *other = caller == a ? b : a; \
            lky_object *func = lobj_get_member(caller, name); \
            if(!func || func->type != LBI_FUNCTION) \
                break; \
            lky_object_function *cfunc = (lky_object_function *)func;\
            if(cfunc->callable.argc != 1 && cfunc->callable.argc != 2) \
                break; \
            return bin_op_exec_custom(cfunc, other, caller == a); \
        } \
    } while(0)

lky_object *bin_op_exec_custom(lky_object_function *func, lky_object *other, char first)
{
    lky_object_seq *seq = lobjb_make_seq_node(other);
    lky_callable c = func->callable;

    if(c.argc == 2)
        seq->next = lobjb_make_seq_node(lobjb_build_int(first));
    
    lky_func_bundle b = MAKE_BUNDLE(func, seq);
    return (lky_object *)c.function(&b);
}

lky_object *lobjb_binary_add(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_add_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_SIGNLETONS(a, b))
        return &lky_nil;
    
    lky_builtin_value v;
    lky_builtin_type t = OBJ_NUM_PROMO(ab, bb);
    switch(t)
    {
        case LBI_FLOAT:
            v.d = OBJ_NUM_UNWRAP(ab) + OBJ_NUM_UNWRAP(bb);
            break;
        case LBI_INTEGER:
            v.i = OBJ_NUM_UNWRAP(ab) + OBJ_NUM_UNWRAP(bb);
            break;
        default:
            break;
    }

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_subtract(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_subtract_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_SIGNLETONS(a, b))
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = OBJ_NUM_PROMO(ab, bb);
    switch(t)
    {
        case LBI_FLOAT:
            v.d = OBJ_NUM_UNWRAP(ab) - OBJ_NUM_UNWRAP(bb);
            break;
        case LBI_INTEGER:
            v.i = OBJ_NUM_UNWRAP(ab) - OBJ_NUM_UNWRAP(bb);
            break;
        default:
            break;
    }

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_multiply(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_multiply_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    lky_builtin_value v;
    lky_builtin_type t;

    if(EITHER_SIGNLETONS(a, b))
        return &lky_nil;

    t = OBJ_NUM_PROMO(ab, bb);
    switch(t)
    {
        case LBI_FLOAT:
            v.d = OBJ_NUM_UNWRAP(ab) * OBJ_NUM_UNWRAP(bb);
            break;
        case LBI_INTEGER:
            v.i = OBJ_NUM_UNWRAP(ab) * OBJ_NUM_UNWRAP(bb);
            break;
        default:
            break;
    }

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_divide(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_divide_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_SIGNLETONS(a, b))
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = OBJ_NUM_PROMO(ab, bb);
    switch(t)
    {
        case LBI_FLOAT:
            v.d = OBJ_NUM_UNWRAP(ab) / OBJ_NUM_UNWRAP(bb);
            break;
        case LBI_INTEGER:
            v.i = OBJ_NUM_UNWRAP(ab) / OBJ_NUM_UNWRAP(bb);
            break;
        default:
            break;
    }

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_modulo(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_modulo_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_SIGNLETONS(a, b))
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = OBJ_NUM_PROMO(ab, bb);
    switch(t)
    {
        case LBI_FLOAT:
            v.d = remainder(OBJ_NUM_UNWRAP(ab), OBJ_NUM_UNWRAP(bb));
            break;
        case LBI_INTEGER:
            v.i = (long)(OBJ_NUM_UNWRAP(a)) % (long)(OBJ_NUM_UNWRAP(b));
            break;
        default:
            break;
    }

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_power(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_power_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_SIGNLETONS(a, b))
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = OBJ_NUM_PROMO(ab, bb);
    switch(t)
    {
        case LBI_FLOAT:
            v.d = pow(OBJ_NUM_UNWRAP(ab), OBJ_NUM_UNWRAP(bb));
            break;
        case LBI_INTEGER:
            v.i = (long)pow(OBJ_NUM_UNWRAP(a), OBJ_NUM_UNWRAP(b));
            break;
        default:
            break;
    }

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_lessthan(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_lt_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_SIGNLETONS(a, b))
        return &lky_nil;

    return LKY_TESTC_FAST(OBJ_NUM_UNWRAP(ab) < OBJ_NUM_UNWRAP(bb));
}

lky_object *lobjb_binary_greaterthan(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_gt_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_SIGNLETONS(a, b))
        return &lky_nil;

    return LKY_TESTC_FAST(OBJ_NUM_UNWRAP(ab) > OBJ_NUM_UNWRAP(bb));
}

lky_object *lobjb_binary_equals(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_equals_");

    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_NIL(a, b))
        return &lky_nil;

    if(OBJ_IS_NUMBER(ab) || OBJ_IS_NUMBER(bb))
    {
        if(!OBJ_IS_NUMBER(ab) || !OBJ_IS_NUMBER(bb))
            return &lky_nil;

        return LKY_TESTC_FAST(OBJ_NUM_UNWRAP(ab) == OBJ_NUM_UNWRAP(bb));
    }

    if(a->type == LBI_BOOL || b->type == LBI_BOOL)
        return LKY_TESTC_FAST(LKY_CTEST_FAST(a) == LKY_CTEST_FAST(b));

    return &lky_nil;
}

lky_object *lobjb_binary_lessequal(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_lte_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_SIGNLETONS(a, b))
        return &lky_nil;

    return LKY_TESTC_FAST(OBJ_NUM_UNWRAP(ab) <= OBJ_NUM_UNWRAP(bb));
}

lky_object *lobjb_binary_greatequal(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_gte_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_SIGNLETONS(a, b))
        return &lky_nil;

    return LKY_TESTC_FAST(OBJ_NUM_UNWRAP(ab) >= OBJ_NUM_UNWRAP(bb));
}

lky_object *lobjb_binary_notequal(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_notequal_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_NIL(a, b))
        return &lky_nil;

    if(OBJ_IS_NUMBER(ab) || OBJ_IS_NUMBER(bb))
    {
        if(!OBJ_IS_NUMBER(ab) || !OBJ_IS_NUMBER(bb))
            return &lky_nil;

        return LKY_TESTC_FAST(OBJ_NUM_UNWRAP(ab) != OBJ_NUM_UNWRAP(bb));
    }

    if(a->type == LBI_BOOL || b->type == LBI_BOOL)
        return LKY_TESTC_FAST(LKY_CTEST_FAST(a) != LKY_CTEST_FAST(b));

    return &lky_nil;
}

lky_object *lobjb_binary_and(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_and_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_SIGNLETONS(a, b))
        return lobjb_build_int(0);

    int vala = a->type == LBI_INTEGER || a->type == LBI_FLOAT ? (int)OBJ_NUM_UNWRAP(ab) : 1;
    int valb = b->type == LBI_INTEGER || b->type == LBI_FLOAT ? (int)OBJ_NUM_UNWRAP(bb) : 1;

    return lobjb_build_int(vala && valb);
}

lky_object *lobjb_binary_or(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_or_");
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(EITHER_SIGNLETONS(a, b))
        return lobjb_build_int(0);

    int vala = a->type == LBI_INTEGER || a->type == LBI_FLOAT ? (int)OBJ_NUM_UNWRAP(ab) : 1;
    int valb = b->type == LBI_INTEGER || b->type == LBI_FLOAT ? (int)OBJ_NUM_UNWRAP(bb) : 1;
    vala = a == &lky_nil ? 0 : vala;
    valb = b == &lky_nil ? 0 : valb;

    return lobjb_build_int(vala || valb);
}

lky_object *lobjb_binary_band(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_bit_and_");

    if(!OBJ_IS_INTEGER(a) || !OBJ_IS_INTEGER(b))
        return &lky_nil;

    long vala = OBJ_NUM_UNWRAP(a);
    long valb = OBJ_NUM_UNWRAP(b);

    return lobjb_build_int(vala & valb);
}

lky_object *lobjb_binary_bor(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_bit_or_");

    if(!OBJ_IS_INTEGER(a) || !OBJ_IS_INTEGER(b))
        return &lky_nil;

    long vala = OBJ_NUM_UNWRAP(a);
    long valb = OBJ_NUM_UNWRAP(b);

    return lobjb_build_int(vala | valb);
}

lky_object *lobjb_binary_bxor(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_bit_xor_");

    if(!OBJ_IS_INTEGER(a) || !OBJ_IS_INTEGER(b))
        return &lky_nil;

    long vala = OBJ_NUM_UNWRAP(a);
    long valb = OBJ_NUM_UNWRAP(b);

    return lobjb_build_int(vala ^ valb);
}

lky_object *lobjb_binary_blshift(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_bit_l_shift_");

    if(!OBJ_IS_INTEGER(a) || !OBJ_IS_INTEGER(b))
        return &lky_nil;

    long vala = OBJ_NUM_UNWRAP(a);
    long valb = OBJ_NUM_UNWRAP(b);

    return lobjb_build_int(vala << valb);
}

lky_object *lobjb_binary_brshift(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_bit_r_shift_");

    if(!OBJ_IS_INTEGER(a) || !OBJ_IS_INTEGER(b))
        return &lky_nil;

    long vala = OBJ_NUM_UNWRAP(a);
    long valb = OBJ_NUM_UNWRAP(b);

    return lobjb_build_int(vala >> valb);
}

lky_object *lobjb_binary_nc(lky_object *a, lky_object *b)
{
    return a == &lky_nil ? b : a;
}
