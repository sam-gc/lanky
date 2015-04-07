/* Lanky -- Scripting Language and Virtual Machine
 * Copyright (C) 2014  Sam Olsen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <string.h>
#include "stl_object.h"
#include "stl_string.h"
#include "stl_table.h"
#include "arraylist.h"
#include "aquarium.h"
#include "lky_gc.h"

struct stlobj_members {
    arraylist keys;
    arraylist vals;
};

static  lky_object *_stlobj_proto = NULL;

lky_object *stlobj_stringify(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object *self = func->bound;
    char buf[100];

    if(self == _stlobj_proto)
        strcpy(buf, "(lky_object | global prototype)");
    else
        sprintf(buf, "(lky_object | %p)", self);

    func->bound = NULL;

    lky_object *retobj = stlstr_cinit(buf);
    return retobj;
}

lky_object *stlobj_responds_to(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *self = func->bound ? func->bound : _stlobj_proto;

    // TODO: There should be some checking here...
    lky_object_custom *c = (lky_object_custom *)args->value;
    char *str = c->data;

    lky_object *o = lobj_get_member(self, str);
    if(!o)
        return &lky_no;

    // Help avoid tagged integer problems
    if(OBJ_IS_INTEGER(o))
        return &lky_no;

    return LKY_TESTC_FAST(o->type == LBI_FUNCTION);
}

lky_object *stlobj_has(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *self = func->bound ? func->bound : _stlobj_proto;

    // TODO: There should be some checking here...
    lky_object_custom *c = (lky_object_custom *)args->value;
    char *str = c->data;

    return LKY_TESTC_FAST(!!lobj_get_member(self, str));
}

lky_object *stlobj_remove(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *self = func->bound;

    lky_object_custom *c = (lky_object_custom *)args->value;
    char *str = c->data;

    hst_remove_key(&self->members, str, NULL, NULL);

    return &lky_nil;
}

lky_object *stlobj_equals(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *self = func->bound;
    lky_object *other = (lky_object *)args->value;

    char is_equal = 0;
    // Handle special case where we are dealing with the
    // prototype.
    if(!func->bound)
        is_equal = other == _stlobj_proto;
    else
        is_equal = other == self;

    func->bound = NULL;

    return LKY_TESTC_FAST(is_equal);
}

void stlobj_members_each(void *key, void *val, void *data)
{
    printf("%s\n", (char *)key);
    struct stlobj_members *m = (struct stlobj_members *)data;
    arr_append(&m->keys, stlstr_cinit((char *)key));
    arr_append(&m->vals, val);
}

void stlobj_members_set_each(void *key, void *val, void *data)
{
    lky_object *o = (lky_object *)data;
    
    char *ch = lobjb_stringify((lky_object *)key, NULL);
    lobj_set_member(o, ch, (lky_object *)val);
}

lky_object *stlobj_set_members(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

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

lky_object *stlobj_members(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    if(args)
        return stlobj_set_members(bundle);
    lky_object *self = func->bound ? func->bound : _stlobj_proto;
    
    struct stlobj_members m;
    m.keys = arr_create(self->members.count + 1);
    m.vals = arr_create(self->members.count + 1);

    hst_for_each(&self->members, stlobj_members_each, &m);
    lky_object *ret = stltab_cinit(&m.keys, &m.vals);

    arr_free(&m.keys);
    arr_free(&m.vals);

    func->bound = NULL;

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

lky_object *stlobj_get_proto()
{
    if(_stlobj_proto)
        return _stlobj_proto;

    // We have to manually create this so as to avoid
    // deep recursion
    lky_object *obj = aqua_request_next_block(sizeof(lky_object));
    obj->type = LBI_CUSTOM;
    obj->mem_count = 0;
    obj->size = sizeof(lky_object);
    obj->members = hst_create();
    obj->members.duplicate_keys = 1;
    gc_add_object(obj);

    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlobj_stringify));
    lobj_set_member(obj, "op_equals_", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlobj_equals));
    lobj_set_member(obj, "members_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlobj_members));
    lobj_set_member(obj, "responds_to", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlobj_responds_to));
    lobj_set_member(obj, "has", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlobj_has));
    lobj_set_member(obj, "remove_member", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlobj_remove));

    _stlobj_proto = obj;

    return obj;
}

void stlobj_seed(lky_object *obj)
{
    //lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlobj_stringify));
    //lobj_set_member(obj, "op_equals_", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlobj_equals));
    lobj_set_member(obj, "proto_", stlobj_get_proto());
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

    lobj_set_member(cls, "model_", stlobj_get_proto());
    lobj_set_member(cls, "new", lobjb_build_func_ex(cls, 0, (lky_function_ptr)stlobj_build));

    _stlobj_class = cls;

    return cls;
}
