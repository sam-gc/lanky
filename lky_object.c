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
    obj->members = trie_new();
    obj->members.free_func = (trie_pointer_function)(&rc_decr);
    // obj->callable = NULL;

    // obj->value = value;
    // alloced++;
    return obj;
}

void lobj_set_member(lky_object *obj, char *member, lky_object *val)
{
    lky_object *old = trie_get(obj->members, member);
    if(old)
        rc_decr(old);

    trie_add(obj->members, member, val);
    rc_incr(val);
}

lky_object *lobj_get_member(lky_object *obj, char *member)
{
    lky_object *val = trie_get(obj->members, member);

    return val;
}

void rc_decr(lky_object *obj)
{
    if(obj == &lky_nil)
        return;
    
    obj->mem_count--;
    // printf("%d (-)\n", obj->mem_count);
    if(!obj->mem_count)
    {
        lobj_dealloc(obj);
    }
}

void lobj_dealloc(lky_object *obj)
{
    printf("(D) ");
    lobjb_print(obj);
    if(obj->type != LBI_CUSTOM)
        lobjb_clean(obj);
    trie_free(obj->members);
    free(obj);
    alloced--;
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
