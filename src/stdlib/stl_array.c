#include <stdlib.h>
#include "stl_array.h"
#include "arraylist.h"
#include "lky_gc.h"

typedef struct {
    arraylist container;
} stlarr_data;

lky_object *stlarr_append(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arr_append(&data->container, args->value);
    return &lky_nil;
}

lky_object *stlarr_get(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    lky_object_builtin *b = args->value;
    long idx = b->value.i;

    return arr_get(&data->container, idx);
}

lky_object *stlarr_for_each(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;
    
    lky_object_function *callback = args->value;

    char useidx = 0;
    if(callback->callable.argc == 2)
        useidx = 1;

    long i;
    for(i = 0; i < list.count; i++)
    {
        lky_object_seq *seq = lobjb_make_seq_node(arr_get(&list, i));
        if(useidx)
            seq->next = lobjb_make_seq_node(lobjb_build_int(i));

        callback->callable.function(seq, callback);
    }

    return &lky_nil;
}

void stlarr_dealloc(lky_object *o)
{
    lky_object_custom *self = (lky_object_custom *)o;
    stlarr_data *data = self->data;
    arr_free(&data->container);
    free(data);
}

void stlarr_save(lky_object *o)
{
    lky_object_custom *self = (lky_object_custom *)o;
    stlarr_data *data = self->data;
    
    long i;
    for(i = 0; i < data->container.count; i++)
        gc_mark_object(arr_get(&data->container, i));

}

lky_object *stlarr_build(lky_object_seq *args, lky_object *func)
{
    lky_object_custom *obj = lobjb_build_custom(sizeof(stlarr_data));
    stlarr_data *data = malloc(sizeof(stlarr_data));
    data->container = arr_create(10);
    obj->data = data;
    
    lobj_set_member(obj, "append", lobjb_build_func_ex(obj, 1, stlarr_append));
    lobj_set_member(obj, "get", lobjb_build_func_ex(obj, 1, stlarr_get));
    lobj_set_member(obj, "forEach", lobjb_build_func_ex(obj, 1, stlarr_for_each));

    obj->freefunc = stlarr_dealloc;
    obj->savefunc = stlarr_save;
    return (lky_object *)obj;
} 


lky_object *stlarr_get_class()
{
    lky_object *clsobj = lobj_alloc();
    lky_callable c;
    c.argc = 0;
    c.function = (lky_function_ptr)stlarr_build;
    clsobj->callable = c;

    return clsobj;
}
