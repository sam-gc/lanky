#ifndef LKYOBJ_BUILTIN_H
#define LKYOBJ_BUILTIN_H

#define OBJ_NUM_UNWRAP(obj) (((lky_object_builtin *)obj)->type == LBI_FLOAT ? ((lky_object_builtin *)obj)->value.d : ((lky_object_builtin *)obj)->value.i)
#define BIN_ARGS lky_object *a, lky_object *b
#define BI_CAST(o, n) lky_object_builtin * n = (lky_object_builtin *) o
#define GET_VA_ARGS(func) (lobj_get_member((lky_object *)func->bucket, "_va_args"))
#define MAKE_VA_ARGS(args, list, ct) do { lky_object_seq *ab = args; int i = 0; for(; args; i++, args = args->next) { if(i < ct) continue; arr_append(&list, args->value);} args = ab; } while(0)

#include "lky_object.h"
#include "arraylist.h"
#include "lky_machine.h"
#include <stdio.h>

//typedef struct interp mach_interp;

typedef void(*lobjb_custom_ex_dealloc_function)(lky_object *o);
typedef void(*lobjb_gc_save_function)(lky_object *o);

typedef union {
    double d;
    long i;
    char *s;
} lky_builtin_value;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *parent;
    lky_object *child;
    
    lky_object *cls;

    lky_callable callable;

    lky_builtin_value value;
} lky_object_builtin;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *parent;
    lky_object *child;
    lky_object *cls;

    lky_callable callable;

    void *data;
    lobjb_custom_ex_dealloc_function freefunc;
    lobjb_gc_save_function savefunc;
} lky_object_custom;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *parent;
    lky_object *child;
    lky_object *cls;

    lky_callable callable;

    long num_constants;
    long num_locals;
    long num_names;

    void **constants;
    void **locals;
    char **names;
    unsigned char *ops;
    long op_len;
    int stack_size;
} lky_object_code;

// TODO: Is this necessary?
typedef struct {
    lky_object *member;
    char *name;
} lky_class_member_wrapper;

struct lky_object_function {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *parent;
    lky_object *child;
    lky_object *cls;

    lky_callable callable;

    arraylist parent_stack;
    lky_object *bucket;
    
    mach_interp *interp;

    lky_object_code *code;
    lky_object *owner;
};

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *parent;
    lky_object *child;
    lky_object *cls;

    lky_callable callable;

    lky_object *parent_cls;
    lky_object *parent_obj;
    lky_object_function *builder;
    char *refname;
} lky_object_class;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *parent;
    lky_object *child;
    lky_object *cls;

    char *name;
    char *text;
} lky_object_error;

lky_object *lobjb_build_int(long value);
lky_object *lobjb_build_float(double value);
lky_object *lobjb_build_error(char *name, char *text);
lky_object_custom *lobjb_build_custom(size_t extra_size);
lky_object *lobjb_build_func(lky_object_code *code, int argc, arraylist inherited, mach_interp *interp);
lky_object *lobjb_build_func_ex(lky_object *owner, int argc, lky_function_ptr ptr);
lky_object *lobjb_build_class(lky_object_function *builder, char *refname, lky_object *parent_class);
lky_object *lobjb_alloc(lky_builtin_type t, lky_builtin_value v);
lky_object *lobjb_default_callable(lky_object_seq *args, lky_object *self);
lky_object *lobjb_default_class_callable(lky_object_seq *args, lky_object *self);

char *lobjb_stringify(lky_object *a);

lky_object *lobjb_unary_load_index(lky_object *obj, lky_object *indexer);
lky_object *lobjb_unary_save_index(lky_object *obj, lky_object *indexer, lky_object *newobj);
lky_object *lobjb_unary_negative(lky_object *obj);

lky_object_seq *lobjb_make_seq_node(lky_object *value);
void lobjb_free_seq(lky_object_seq *seq);

void lobjb_print_object(lky_object *a);
void lobjb_print(lky_object *a);
char lobjb_quick_compare(lky_object *a, lky_object *b);
lky_object *lobjb_num_to_string(lky_object *a);

lky_object_code *lobjb_load_file(char *name);

void lobjb_serialize(lky_object *obj, FILE *f);
lky_object *lobjb_deserialize(FILE *f);

void lobjb_clean(lky_object *a);


#endif
