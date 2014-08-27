#ifndef STL_META_H
#define STL_META_H

#include "lkyobj_builtin.h"

void run_repl(mach_interp *interp);
lky_object *stlmeta_get_class(mach_interp *interp);
lky_object *stlmeta_examine(lky_object_seq *args, lky_object_function *func);

#endif