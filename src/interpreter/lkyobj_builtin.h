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

#ifndef LKYOBJ_BUILTIN_H
#define LKYOBJ_BUILTIN_H

#define OBJ_NUM_UNWRAP(obj) (((uintptr_t)(obj) & 1) ? ((long)(obj) >> 8 & 0x00FFFFFFF) : (((lky_object_builtin *)obj)->type == LBI_FLOAT ? ((lky_object_builtin *)obj)->value.d : ((lky_object_builtin *)obj)->value.i))
#define OBJ_IS_NUMBER(obj) (((uintptr_t)(obj) & 1) || obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)
#define OBJ_IS_INTEGER(obj) (((uintptr_t)(obj) & 1) || obj->type == LBI_INTEGER)
#define BIN_ARGS lky_object *a, lky_object *b
#define BI_CAST(o, n) lky_object_builtin * n = (lky_object_builtin *) o
#define GET_VA_ARGS(func) (lobj_get_member((lky_object *)func->bucket, "_va_args"))
#define MAKE_VA_ARGS(args, list, ct) do { lky_object_seq *ab = args; int i = 0; for(; args; i++, args = args->next) { if(i < ct) continue; arr_append(&list, args->value);} args = ab; } while(0)
#define LKY_NEXT_ITERABLE(obj) (obj->type != LBI_ITERABLE ? NULL :\
        (((lky_object_iterable *)(obj))->index < ((lky_object_iterable *)(obj))->store->count ?\
         ((lky_object_iterable *)(obj))->store->items[((lky_object_iterable *)(obj))->index++] : NULL))
#define LKY_TEST_FAST(cond)\
    (!cond || cond == &lky_nil ? &lky_no : (OBJ_IS_NUMBER(cond) ? (!!OBJ_NUM_UNWRAP(cond) ? &lky_yes : &lky_no) :\
                                            (cond->type == LBI_BOOL ? cond : &lky_yes)))
#define LKY_CTEST_FAST(cond)\
    (!cond || cond == &lky_nil ? 0 : (OBJ_IS_NUMBER(cond) ? !!OBJ_NUM_UNWRAP(cond) : (cond->type == LBI_BOOL ? cond == &lky_yes : 1)))
#define LKY_TESTC_FAST(val) (val ? &lky_yes : &lky_no)
#define LKY_ARGS(...) lobjb_build_args(__VA_ARGS__, NULL)

#include "lky_object.h"
#include "arraylist.h"
#include "lky_machine.h"
#include "class_builder.h"
#include <stdio.h>
#include <stdint.h>

//typedef struct interp mach_interp;

typedef enum {
    CGC_MARK,
    CGC_FREE
} lky_class_gc_type;

typedef void(*lobjb_custom_ex_dealloc_function)(lky_object *o);
typedef void(*lobjb_gc_save_function)(lky_object *o);
typedef void(*lobjb_void_ptr_function)(void *o, lky_class_gc_type opt);

typedef union {
    double d;
    long i;
    void *b;
} lky_builtin_value;

typedef struct {
    unsigned type : 4;
    unsigned mem_count : 1;
    
    lky_builtin_value value;
    lobjb_void_ptr_function on_gc;
} lky_object_builtin;

typedef struct {
    unsigned type : 4;
    unsigned mem_count : 1;

    int index;
    arraylist *store;
    lky_object *owner;
} lky_object_iterable;

typedef struct {
    unsigned type : 4;
    unsigned mem_count : 1;

    hashtable members;
    lky_object *cls;

    lky_callable callable;

    void *data;
    lobjb_custom_ex_dealloc_function freefunc;
    lobjb_gc_save_function savefunc;
} lky_object_custom;

typedef struct {
    unsigned type : 4;
    unsigned mem_count : 1;

    long num_constants;
    long num_locals;
    long num_names;

    void **constants;
    void **locals;
    char **names;
    unsigned char *ops;
    long op_len;
    int stack_size;
    int catch_size;

    char *refname;
} lky_object_code;

// TODO: Is this necessary?
typedef struct {
    lky_object *member;
    char *name;
} lky_class_member_wrapper;

struct lky_object_function {
    unsigned type : 4;
    unsigned mem_count : 1;

    hashtable members;

    lky_callable callable;

    arraylist parent_stack;
    lky_object *bucket;
    
    mach_interp *interp;

    lky_object_code *code;
    lky_object *owner;
    lky_object *bound;
    char *refname;
};

typedef struct {
    unsigned type : 4;
    unsigned mem_count : 1;

    hashtable members;

    lky_callable callable;

    lky_object *parent_cls;
    lky_object *parent_obj;
    lky_object_function *builder;
    char *refname;
} lky_object_class;

typedef struct {
    unsigned type : 4;
    unsigned mem_count : 1;

    hashtable members;
    lky_object *cls;

    char *name;
    char *text;
} lky_object_error;

extern int lobjb_uses_pointer_tags_;

lky_object *lobjb_call(lky_object *func, lky_object_seq *args, struct interp *interp);
lky_object *lobjb_build_int(long value);
lky_object *lobjb_build_float(double value);
lky_object *lobjb_build_blob(void *ptr, lobjb_void_ptr_function gc);
lky_object *lobjb_build_error(char *name, char *text);
lky_object *lobjb_build_iterable(lky_object *owner, struct interp *interp);
lky_object_custom *lobjb_build_custom(size_t extra_size);
lky_object *lobjb_build_func(lky_object_code *code, int argc, arraylist inherited, mach_interp *interp);
lky_object *lobjb_build_func_ex(lky_object *owner, int argc, lky_function_ptr ptr);
lky_object *lobjb_alloc(lky_builtin_type t, lky_builtin_value v);
lky_object *lobjb_default_callable(lky_func_bundle *bundle);

lky_object *lobjb_get_exception_class();

char *lobjb_stringify(lky_object *a, struct interp *interp);

lky_object *lobjb_unary_load_index(lky_object *obj, lky_object *indexer, struct interp *interp);
lky_object *lobjb_unary_save_index(lky_object *obj, lky_object *indexer, lky_object *newobj, struct interp *interp);
lky_object *lobjb_unary_negative(lky_object *obj);

lky_object *lobjb_iterable_get_next(lky_object *obj);

lky_object_seq *lobjb_make_seq_node(lky_object *value);
void lobjb_free_seq(lky_object_seq *seq);
lky_object_seq *lobjb_build_args(lky_object *arg, ...);

void lobjb_print_object(lky_object *a, struct interp *interp);
void lobjb_print(lky_object *a, struct interp *interp);
char lobjb_quick_compare(lky_object *a, lky_object *b);
lky_object *lobjb_num_to_string(lky_object *a);

lky_object *lobjb_test(lky_object *cond);
int lobjb_ctest(lky_object *cond);

void lobjb_clean(lky_object *a);


#endif
