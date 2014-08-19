#include <stdlib.h>
#include "stl_array.h"
#include "arraylist.h"
#include "lky_gc.h"
#include "lky_machine.h"
#include "mach_binary_ops.h"

#define FAIL_CHECK(check, name, text) do { if(check) { mach_halt_with_err(lobjb_build_error(name, text)); return &lky_nil; } }while(0);

typedef struct {
    arraylist container;
} stlarr_data;

lky_object *stlarr_append(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arr_append(&data->container, args->value);
    lobj_set_member(self, "count", lobjb_build_int(data->container.count));
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

lky_object *stlarr_contains(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;
    lky_object *a = args->value;

    int toret = 0;

    long i;
    for(i = 0; i < list.count; i++)
    {
        lky_object *b = arr_get(&list, i);
        lky_object *result = lobjb_binary_equals(a, b);
        if(result->type == LBI_INTEGER || result->type == LBI_FLOAT)
        {
            toret = !!OBJ_NUM_UNWRAP(result);
            break;
        }
        
        if(result->type == &lky_nil)
            continue;

        toret = 1;
        break;
    }

    return lobjb_build_int(toret);
}

lky_object *stlarr_for_each(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;
    
    lky_object_function *callback = args->value;
    FAIL_CHECK(callback->type != LBI_FUNCTION, "MismatchedType", "Expected function type");
    FAIL_CHECK(!callback->callable.argc || callback->callable.argc > 2, "MismatchedType", "Expected function with 1 or 2 arguments");

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

lky_object *stlarr_cinit(arraylist inlist)
{
    lky_object_custom *obj = lobjb_build_custom(sizeof(stlarr_data));
    stlarr_data *data = malloc(sizeof(stlarr_data));
    data->container = inlist;
    obj->data = data;
    
    lobj_set_member(obj, "append", lobjb_build_func_ex(obj, 1, stlarr_append));
    lobj_set_member(obj, "get", lobjb_build_func_ex(obj, 1, stlarr_get));
    lobj_set_member(obj, "forEach", lobjb_build_func_ex(obj, 1, stlarr_for_each));
    lobj_set_member(obj, "count", lobjb_build_int(0));
    lobj_set_member(obj, "contains", lobjb_build_func_ex(obj, 1, stlarr_contains));

    obj->freefunc = stlarr_dealloc;
    obj->savefunc = stlarr_save;
    return (lky_object *)obj;
}

lky_object *stlarr_build(lky_object_seq *args, lky_object *func)
{
    return stlarr_cinit(arr_create(10));
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
