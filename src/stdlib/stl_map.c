#include <stdlib.h>
#include "stl_map.h"
#include "trie.h"

typedef struct {
    Trie_t container;
} stlmap_object_data;

lky_object *stlmap_build(lky_object_seq *args, lky_object *func)
{
    return NULL;
}

lky_object *stlmap_get_class()
{
    lky_object *clsobj = lobj_alloc();

    lky_callable c;
    c.argc = 0;
    c.function = (lky_function_ptr)stlmap_build;
    clsobj->callable = c;

    return clsobj;
}