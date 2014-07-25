#ifndef LKYOBJ_BUILTIN_H
#define LKYOBJ_BUILTIN_H

#define OBJ_NUM_UNWRAP(obj) (((lky_object_builtin *)obj)->type == LBI_FLOAT ? ((lky_object_builtin *)obj)->value.d : ((lky_object_builtin *)obj)->value.i)

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

    lky_builtin_value value;
} lky_object_builtin;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    arraylist members;

    long num_constants;
    long num_locals;

    void **constants;
    void **locals;
    char *ops;
    long op_len;
} lky_object_code;

lky_object *lobjb_build_int(long value);
lky_object *lobjb_build_float(double value);
lky_object *lobjb_alloc(lky_builtin_type t, lky_builtin_value v);
lky_object *lobjb_binary_add(lky_object *a, lky_object *b);
lky_object *lobjb_binary_subtract(lky_object *a, lky_object *b);
lky_object *lobjb_binary_multiply(lky_object *a, lky_object *b);
lky_object *lobjb_binary_divide(lky_object *a, lky_object *b);
lky_object *lobjb_binary_lessthan(lky_object *a, lky_object *b);
void lobjb_print(lky_object *a);
char lobjb_quick_compare(lky_object *a, lky_object *b);

lky_object_code *lobjb_load_file(char *name);

void lobjb_serialize(lky_object *obj, FILE *f);
lky_object *lobjb_deserialize(FILE *f);

void lobjb_clean(lky_object *a);


#endif