#ifndef LKY_OBJECT_H
#define LKY_OBJECT_H

#include <stdlib.h>
#include "trie.h"

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
    LBI_CUSTOM,
    LBI_CUSTOM_EX,
    LBI_ERROR
} lky_builtin_type;

typedef struct lky_object *(*lky_function_ptr)(struct lky_object_seq *args, struct lky_object *self);

typedef struct {
    int argc;
    lky_function_ptr function;
} lky_callable;

typedef struct lky_object_seq {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;

    lky_callable callable;

    struct lky_object *value;
    struct lky_object_seq *next;
} lky_object_seq;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;

    lky_callable callable;
} lky_object;

lky_object *lobj_alloc();
void lobj_set_member(lky_object *obj, char *member, lky_object *val);
lky_object *lobj_get_member(lky_object *obj, char *member);
void rc_decr(lky_object *obj);
void rc_incr(lky_object *obj);
void lobj_dealloc(lky_object *obj);
void print_alloced();

extern lky_object lky_nil;

#endif
