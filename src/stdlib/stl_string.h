#ifndef STL_STRING_H
#define STL_STRING_H

#include "lkyobj_builtin.h"

lky_object *stlstr_cinit(char *str);
lky_object *stlstr_fmt_ext(char *mestr, arraylist list);
lky_object *stlstr_class();
//lky_object *stlstr_get_class();

#endif
