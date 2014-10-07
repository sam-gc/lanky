#ifndef STL_ARRAY_H
#define STL_ARRAY_H

#include "lkyobj_builtin.h"

lky_object *stlarr_cinit(arraylist inlist);
arraylist stlarr_unwrap(lky_object *obj);
lky_object *stlarr_get_class();

#endif
