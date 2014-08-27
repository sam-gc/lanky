#include <stdlib.h>
#include <string.h>
#include "stl_array.h"
#include "arraylist.h"
#include "lky_gc.h"
#include "lky_machine.h"
#include "stl_string.h"
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

lky_object *stlarr_set(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_function *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;

    lky_object *indexer = args->value;
    lky_object *newobj = args->next->value;

    if(indexer->type != LBI_FLOAT && indexer->type != LBI_INTEGER)
    {
        mach_halt_with_err(lobjb_build_error("MismatchedType", "You can only index an array with an int or a float!"));
        return &lky_nil;
    }

    lky_object_builtin *b = (lky_object_builtin *)indexer;
    long idx = OBJ_NUM_UNWRAP(b);
    if(idx >= list.count || idx < 0)
    {
        mach_halt_with_err(lobjb_build_error("IndexOutOfBounds", "The given index is not valid for the array."));
        return &lky_nil;
    }

    list.items[idx] = newobj;

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
            if(toret)
                break;
        }

        if(result->type == &lky_nil)
            continue;

//        toret = 1;
//        break;
    }

    return lobjb_build_int(toret);
}

lky_object *stlarr_for_each(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;

//    gc_add_root_object(self);

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

//    gc_remove_root_object(self);

    return &lky_nil;
}

lky_object *stlarr_index_of(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;
    lky_object *a = args->value;

    long i;
    for(i = 0; i < list.count; i++)
    {
        lky_object *b = arr_get(&list, i);
        lky_object *result = lobjb_binary_equals(a, b);
        if(result->type == LBI_INTEGER || result->type == LBI_FLOAT)
        {
            if(OBJ_NUM_UNWRAP(result))
                return lobjb_build_int(i);
            continue;
        }

        if(result->type == &lky_nil)
            continue;

        return lobjb_build_int(i);
    }

    return lobjb_build_int(-1);
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
    {
        lky_object *obj = arr_get(&data->container, i);
        if(!obj)
            continue;
        
        gc_mark_object(arr_get(&data->container, i));
    }

}

lky_object *stlarr_remove_at(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    
    arraylist *list = &data->container;
    
    long idx = OBJ_NUM_UNWRAP(args->value);
    
    lky_object *obj = arr_get(list, idx);
    arr_remove(list, NULL, idx);
    
    lobj_set_member(self, "count", lobjb_build_int(data->container.count));
    
    return obj;
}

lky_object *stlarr_stringify(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;
    
    char *innards[list.count];
    size_t tlen = 0;
    
    int i;
    for(i = 0; i < list.count; i++)
    {
        lky_object_custom *strobj = NULL;
        lky_object *obj = arr_get(&list, i);
        lky_object_function *f = (lky_object_function *)lobj_get_member(obj, "stringify_");
        
        if(!f)
        {
            char str[100];
            
            if(obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)
                strobj = (lky_object_custom *)lobjb_num_to_string(obj);
            else
            {
                sprintf(str, "%p", obj);
                strobj = (lky_object_custom *)stlstr_cinit(str);
            }
        }
        else
        {
            strobj = (lky_object_custom *)(f->callable.function)(NULL, (struct lky_object *)f);
        }
        
        char *str = strobj->data;
        
        innards[i] = str;
        tlen += strlen(str);
    }
    
    // Adding to the total length:
    //      brackets      commas & spaces                           null termination
    tlen +=    4     +  2 * (list.count > 0 ? list.count - 1 : 0)  +       1;
    
    char *str = malloc(tlen);
    strcpy(str, "[ ");
    
    for(i = 0; i < list.count - 1; i++)
    {
        strcat(str, innards[i]);
        strcat(str, ", ");
    }
    
    strcat(str, innards[i]);
    strcat(str, " ]");
    
    lky_object *ret = stlstr_cinit(str);
    free(str);
    
    return ret;
}

lky_object *stlarr_cinit(arraylist inlist)
{
    lky_object_custom *obj = lobjb_build_custom(sizeof(stlarr_data));
    stlarr_data *data = malloc(sizeof(stlarr_data));
    data->container = inlist;
    obj->data = data;

    lky_object *getter = lobjb_build_func_ex(obj, 1, stlarr_get);
    lky_object *setter = lobjb_build_func_ex(obj, 2, stlarr_set);

    lobj_set_member(obj, "append", lobjb_build_func_ex(obj, 1, stlarr_append));
    lobj_set_member(obj, "get", getter); // We want to let people directly call get
    lobj_set_member(obj, "op_get_index_", getter); // For the builtin getting syntax
    lobj_set_member(obj, "set", setter); // Direct call
    lobj_set_member(obj, "op_set_index_", setter); // For the builtin setting syntax
    lobj_set_member(obj, "forEach", lobjb_build_func_ex(obj, 1, stlarr_for_each));
    lobj_set_member(obj, "count", lobjb_build_int(inlist.count));
    lobj_set_member(obj, "contains", lobjb_build_func_ex(obj, 1, stlarr_contains));
    lobj_set_member(obj, "indexOf", lobjb_build_func_ex(obj, 1, stlarr_index_of));
    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, stlarr_stringify));
    lobj_set_member(obj, "removeAt", lobjb_build_func_ex(obj, 1, stlarr_remove_at));

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
