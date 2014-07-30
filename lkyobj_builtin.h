#ifndef LKYOBJ_BUILTIN_H
#define LKYOBJ_BUILTIN_H

#define OBJ_NUM_UNWRAP(obj) (((lky_object_builtin *)obj)->type == LBI_FLOAT ? ((lky_object_builtin *)obj)->value.d : ((lky_object_builtin *)obj)->value.i)
#define BIN_ARGS lky_object *a, lky_object *b

#include "lky_object.h"
#include <stdio.h>

typedef union {
    double d;
    long i;
    char *s;
} lky_builtin_value;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    arraylist members;

    lky_callable callable;

    lky_builtin_value value;
} lky_object_builtin;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    arraylist members;

    lky_callable callable;

    long num_constants;
    long num_locals;

    void **constants;
    void **locals;
    char *ops;
    long op_len;
} lky_object_code;



typedef struct {
    lky_builtin_type type;
    int mem_count;
    arraylist members;

    lky_callable callable;

    int argc;
    lky_object_code *code;
} lky_object_function;

lky_object *lobjb_build_int(long value);
lky_object *lobjb_build_float(double value);
lky_object *lobjb_build_func(lky_object_code *code, int argc);
lky_object *lobjb_alloc(lky_builtin_type t, lky_builtin_value v);
lky_object *lobjb_binary_add(lky_object *a, lky_object *b);
lky_object *lobjb_binary_subtract(lky_object *a, lky_object *b);
lky_object *lobjb_binary_multiply(lky_object *a, lky_object *b);
lky_object *lobjb_binary_divide(lky_object *a, lky_object *b);
lky_object *lobjb_binary_modulo(lky_object *a, lky_object *b);
lky_object *lobjb_binary_lessthan(lky_object *a, lky_object *b);
lky_object *lobjb_binary_greaterthan(lky_object *a, lky_object *b);
lky_object *lobjb_binary_equals(lky_object *a, lky_object *b);
lky_object *lobjb_binary_lessequal(lky_object *a, lky_object *b);
lky_object *lobjb_binary_greatequal(lky_object *a, lky_object *b);
lky_object *lobjb_binary_notequal(lky_object *a, lky_object *b);
lky_object *lobjb_default_callable(lky_object_seq *args, lky_object *self);

lky_object_seq *lobjb_make_seq_node(lky_object *value);
void lobjb_free_seq(lky_object_seq *seq);

void lobjb_print(lky_object *a);
char lobjb_quick_compare(lky_object *a, lky_object *b);

lky_object_code *lobjb_load_file(char *name);

void lobjb_serialize(lky_object *obj, FILE *f);
lky_object *lobjb_deserialize(FILE *f);

void lobjb_clean(lky_object *a);


#endif