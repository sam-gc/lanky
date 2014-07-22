#ifndef LKY_OBJECT_H
#define LKY_OBJECT_H

#include "arraylist.h"

// #define INCREF(obj) (rc_decr(obj))
typedef enum {
    LBI_FLOAT,
    LBI_INTEGER,
    LBI_STRING,
    LBI_NIL,
    LBI_CUSTOM
} lky_builtin_type;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    arraylist members;
} lky_object;

lky_object *lobj_alloc();
void rc_decr(lky_object *obj);
void rc_incr(lky_object *obj);

extern lky_object lky_nil;

#endif