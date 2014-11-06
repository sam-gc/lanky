#ifndef STL_TABLE_H
#define STL_TABLE_H

#include "lkyobj_builtin.h"
#include "arraylist.h"

lky_object *stltab_get_class();
lky_object *stltab_cinit(arraylist *keys, arraylist *vals);

#endif
