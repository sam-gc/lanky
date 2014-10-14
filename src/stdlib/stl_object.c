#include <stdlib.h>
#include <string.h>
#include "stl_object.h"
#include "stl_string.h"

lky_object *stlobj_stringify(lky_object_seq *args, lky_object_function *func)
{
    lky_object *self = func->owner;
    char *buf = malloc(100);
    sprintf(buf, "(lky_object | %p)", self);
    lky_object *retobj = stlstr_cinit(buf);
    free(buf);
    return retobj;
}

lky_object *stlobj_equals(lky_object_seq *args, lky_object_function *func)
{
    // TODO: Using func->owner for self is problematic
    // for subclassing.
    lky_object *self = func->owner;
    lky_object *other = (lky_object *)args->value;

    char is_equal = 0;
    for(; other; other = (lky_object *)other->parent)
        if(other == self)
            is_equal = 1;

    return lobjb_build_int(is_equal);
}

lky_object *stlobj_cinit()
{
    lky_object_custom *self = lobjb_build_custom(0);
    lky_object *obj = (lky_object *)self;

    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlobj_stringify));
    lobj_set_member(obj, "op_equals_", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlobj_equals));

    return obj;
}

lky_object *stlobj_build(lky_object_seq *args, lky_object_function *function)
{
    return stlobj_cinit();
}

static lky_object *_stlobj_class = NULL;
lky_object *stlobj_get_class()
{
    if(_stlobj_class)
        return _stlobj_class;

    lky_object *cls = lobj_alloc();
    lky_callable c;
    c.argc = 0;
    c.function = (lky_function_ptr)stlobj_build;
    cls->callable = c;

    _stlobj_class = cls;

    return cls;
}
