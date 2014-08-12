#ifndef LKY_GC_H
#define LKY_GC_H

#include "lkyobj_builtin.h"

void gc_init();
void gc_add_root_object(lky_object *obj);
void gc_remove_root_object(lky_object *obj);
void gc_add_root_stack(void **stack, int size);
void gc_remove_root_stack(void **stack);
void gc_add_object(lky_object *obj);
void gc_gc();

#endif
