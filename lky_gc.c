#include "lky_gc.h"
#include "arraylist.h"

typedef struct {
    arraylist pool;
    arraylist roots;
    size_t max_size;
} gc_bundle;

gc_bundle bundle;

void gc_init()
{
    bundle.pool = arr_create(100);
    bundle.roots = arr_create(10);
    bundle.max_size = 16000000;
}

void gc_add_root_object(lky_object *obj)
{

}
    
void gc_add_object(lky_object *obj, size_t size)
{

}
