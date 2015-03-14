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

#ifndef LKY_OBJECT_H
#define LKY_OBJECT_H

#include <stdlib.h>
#include "hashtable.h"

// #define INCREF(obj) (rc_decr(obj))
struct lky_object_seq;
struct lky_object;

typedef enum {
    LBI_FLOAT,
    LBI_INTEGER,
    LBI_STRING,
    LBI_SEQUENCE,
    LBI_NIL,
    LBI_FUNCTION,
    LBI_CLASS,
    LBI_CODE,
    LBI_ITERABLE,
    LBI_CUSTOM,
    LBI_CUSTOM_EX,
    LBI_ERROR,
    LBI_BOOL
} lky_builtin_type;

struct lky_func_bundle;
typedef struct lky_object *(*lky_function_ptr)(struct lky_func_bundle *);

typedef struct {
    int argc;
    lky_function_ptr function;
} lky_callable;

typedef struct lky_object_seq {
    lky_builtin_type type;
    int mem_count;
    size_t size;

    struct lky_object *value;
    struct lky_object_seq *next;
} lky_object_seq;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    hashtable members;

    struct lky_object *cls;

    lky_callable callable;
} lky_object;

typedef struct lky_func_bundle {
    lky_object *func;
    lky_object_seq *args;

    struct interp *interp;
} lky_func_bundle;

lky_object *lobj_alloc();
void lobj_set_member(lky_object *obj, char *member, lky_object *val);
lky_object *lobj_get_member(lky_object *obj, char *member);
void lobj_dealloc(lky_object *obj);
void print_alloced();
void lobj_set_class(lky_object *obj, lky_object *cls);
char lobj_is_of_class(lky_object *obj, void *cls);
char lobj_have_same_class(lky_object *a, lky_object *b);
char *lobj_stringify(lky_object *obj, struct interp *interp);

extern lky_object lky_nil;
extern lky_object lky_yes;
extern lky_object lky_no;

#define MAKE_BUNDLE(f, a, i) {\
    .func = (lky_object *)(f),\
    .args = (lky_object_seq *)(a),\
    .interp = i\
}

#define BUW_FUNC(b) ((lky_object_function *)b->func)
#define BUW_ARGS(b) ((lky_object_seq *)b->args)
#define BUW_INTERP(b) ((struct interp *)b->interp)

#endif
