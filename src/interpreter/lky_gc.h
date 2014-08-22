#ifndef LKY_GC_H
#define LKY_GC_H

#include "lkyobj_builtin.h"

void gc_init();
void gc_pause();
void gc_resume();
void gc_add_func_stack(stackframe *frame);
void gc_add_root_object(lky_object *obj);
void gc_remove_root_object(lky_object *obj);
void gc_add_object(lky_object *obj);
void gc_gc();
void gc_mark_object(lky_object *o);

#endif
