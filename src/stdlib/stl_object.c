#include <stdlib.h>
#include <string.h>
#include "stl_object.h"
#include "stl_string.h"
#include "stl_table.h"
#include "arraylist.h"

struct stlobj_members {
    arraylist keys;
    arraylist vals;
};

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

void stlobj_members_each(void *key, void *val, void *data)
{
    printf("%s\n", key);
    struct stlobj_members *m = (struct stlobj_members *)data;
    arr_append(&m->keys, stlstr_cinit((char *)key));
    arr_append(&m->vals, val);
}

void stlobj_members_set_each(void *key, void *val, void *data)
{
    lky_object *o = (lky_object *)data;
    
    char *ch = lobjb_stringify((lky_object *)key);
    lobj_set_member(o, ch, (lky_object *)val);
}

lky_object *stlobj_set_members(lky_object_seq *args, lky_object_function *func)
{
    lky_object *self = func->owner;

    lky_object *dict = (lky_object *)args->value;
    int append = 0;
    if(args->next)
        append = OBJ_NUM_UNWRAP(args->next->value);

    hashtable ht = stltab_unwrap(dict);

    if(!append)
    {
        hst_free(&self->members);
        self->members = hst_create();
        stlobj_seed(self);
    }

    hst_for_each(&ht, stlobj_members_set_each, self);

    return &lky_nil;
}

lky_object *stlobj_members(lky_object *args, lky_object_function *func)
{
    if(args)
        return stlobj_set_members((lky_object_seq *)args, func);
    lky_object *self = func->owner;
    
    struct stlobj_members m;
    m.keys = arr_create(self->members.count + 1);
    m.vals = arr_create(self->members.count + 1);

    hst_for_each(&self->members, stlobj_members_each, &m);
    lky_object *ret = stltab_cinit(&m.keys, &m.vals);

    arr_free(&m.keys);
    arr_free(&m.vals);

    return ret;
}

lky_object *stlobj_cinit()
{
    lky_object_custom *self = lobjb_build_custom(0);
    lky_object *obj = (lky_object *)self;

    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlobj_stringify));
    lobj_set_member(obj, "op_equals_", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlobj_equals));

    return obj;
}

void stlobj_seed(lky_object *obj)
{
    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlobj_stringify));
    lobj_set_member(obj, "op_equals_", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlobj_equals));
    lobj_set_member(obj, "members_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlobj_members));
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
