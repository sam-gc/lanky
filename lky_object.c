#include "lky_object.h"
#include "lkyobj_builtin.h"
#include <stdio.h>
#include <stdlib.h>

lky_object lky_nil = {LBI_NIL, 0, {NULL}};

int alloced = 0;
lky_object *lobj_alloc()
{
    lky_object *obj = malloc(sizeof(lky_object));
    obj->type = LBI_CUSTOM;
    obj->mem_count = 0;
    obj->members = arr_create(10);
    // obj->callable = NULL;

    // obj->value = value;
    // alloced++;
    return obj;
}

void rc_decr(lky_object *obj)
{
    if(obj == &lky_nil)
        return;
    
    obj->mem_count--;
    // printf("%d (-)\n", obj->mem_count);
    if(!obj->mem_count)
    {
        // printf("Freeing : ");
        // lobjb_print(obj);
        if(obj->type != LBI_CUSTOM)
            lobjb_clean(obj);
        arr_free(&(obj->members));
        free(obj);
        alloced--;
    }
}

void rc_incr(lky_object *obj)
{
    (obj->mem_count)++;
    // printf("%d (+)\n", obj->mem_count);
}

void print_alloced()
{
    printf(">>>> %d\n", alloced);
}