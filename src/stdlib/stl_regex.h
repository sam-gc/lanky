#ifndef STL_REGEX_H
#define STL_REGEX_H

#include "lky_object.h"
#include "regex.h"

lky_object *stlrgx_cinit(char *pattern, char *flags);
lky_object *stlrgx_get_class();
rgx_regex *stlrgx_unwrap(lky_object *obj);

#endif
