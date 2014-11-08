#ifndef STL_TABLE_H
#define STL_TABLE_H

#include "lkyobj_builtin.h"
#include "arraylist.h"
#include "hashtable.h"

lky_object *stltab_get_class();
lky_object *stltab_cinit(arraylist *keys, arraylist *vals);
hashtable stltab_unwrap(lky_object *obj);
long stltab_autohash(void *key, void *data);
int stltab_autoequ(void *a, void *b);

#endif
