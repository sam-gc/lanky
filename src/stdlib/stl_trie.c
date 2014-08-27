#include "lky_gc.h"
#include "stl_trie.h"
#include "trie.h"

typedef struct {
    Trie_t container;
} stltrie_object_data;

lky_object *stltrie_put(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stltrie_object_data *data = self->data;
    Trie_t trie = data->container;
    
    lky_object_custom *name = (lky_object_custom *)args->value;
    trie_add(&trie, name->data, (void *)1);
    
    return &lky_nil;
}

lky_object *stltrie_contains(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stltrie_object_data *data = self->data;
    Trie_t trie = data->container;
    
    lky_object_custom *name = (lky_object_custom *)args->value;
    int cts = !!trie_get(&trie, name->data);
    
    return lobjb_build_int(cts);
}

lky_object *stltrie_has_path(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stltrie_object_data *data = self->data;
    Trie_t trie = data->container;
    
    lky_object_custom *name = (lky_object_custom *)args->value;
    int cts = trie_contains_path(&trie, name->data);
    
    return lobjb_build_int(cts);
}

//void stltrie_save(lky_object *o)
//{
//    lky_object_custom *self = (lky_object_custom *)o;
//    stltrie_object_data *data = self->data;
//    
//    Trie_t trie = data->container;
//    
//    trie_for_each(&trie, (trie_pointer_function)&gc_mark_object);
//}

void stltrie_free(lky_object *o)
{
    lky_object_custom *self = (lky_object_custom *)o;
    stltrie_object_data *data = self->data;
    
    Trie_t trie = data->container;
    
    trie_free(trie);
    
    free(data);
}

lky_object *stltrie_build(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *cobj = lobjb_build_custom(sizeof(stltrie_object_data));
    
    stltrie_object_data *data = malloc(sizeof(stltrie_object_data));
    data->container = trie_new();
    
    cobj->data = data;
    cobj->freefunc = stltrie_free;
    
    lky_object *obj = (lky_object *)cobj;
    
    lobj_set_member(obj, "contains", (lky_object *)lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltrie_contains));
    lobj_set_member(obj, "hasPath", (lky_object *)lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltrie_has_path));
    lobj_set_member(obj, "put", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltrie_put));
    
    return (lky_object *)obj;
}

lky_object *stltrie_get_class()
{
    lky_object *clsobj = lobj_alloc();
    
    lky_callable c;
    c.function = (lky_function_ptr)stltrie_build;
    c.argc = 0;
    
    clsobj->callable = c;
    
    return clsobj;
}