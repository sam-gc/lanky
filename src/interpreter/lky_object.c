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

#include "lky_object.h"
#include "lkyobj_builtin.h"
#include "lky_gc.h"
#include "stl_string.h"
#include "stl_object.h"
#include "stl_table.h"
#include "aquarium.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lky_object lky_nil = {LBI_NIL, 0, sizeof(lky_object), {0, 0, 0, NULL}, NULL, {0, NULL}};
lky_object lky_yes = {LBI_BOOL, 1, sizeof(lky_object), {0, 0, 0, NULL}, NULL, {0, NULL}};
lky_object lky_no = {LBI_BOOL, 1, sizeof(lky_object), {0, 0, 0, NULL}, NULL, {0, NULL}};

int alloced = 0;
lky_object *lobj_alloc()
{
    lky_object *obj = aqua_request_next_block(sizeof(lky_object));
    obj->type = LBI_CUSTOM;
    obj->mem_count = 0;
    obj->size = sizeof(lky_object);
    obj->members = hst_create();
    obj->members.duplicate_keys = 1;
    gc_add_object(obj);

    stlobj_seed(obj);
    return obj;
}

void lobj_set_member(lky_object *obj, char *member, lky_object *val)
{
    if(((uintptr_t)(obj) & 1) || obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)
        return;

    hst_put(&obj->members, member, val, NULL, NULL);
}

lky_object *lobj_get_member(lky_object *obj, char *member)
{
    if(!obj || ((uintptr_t)(obj) & 1) || obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)
        return NULL;

    lky_object *val = hst_get(&obj->members, member, NULL, NULL);
    if(!val && obj->type != LBI_FUNCTION && obj->type != LBI_CLASS && !!strcmp(member, "proto_"))
    {
        lky_object *proto = hst_get(&obj->members, "proto_", NULL, NULL);
        if(proto)
        {
            lky_object *m = lobj_get_member(proto, member);
            if(!m)
                return NULL;
            if(!OBJ_IS_INTEGER(m) && m->type == LBI_FUNCTION)
                ((lky_object_function *)m)->bound = obj;

            return m;
        }
    }
    else if(val && !OBJ_IS_INTEGER(val) && val && val->type == LBI_FUNCTION)
        ((lky_object_function *)val)->bound = obj;

    return val;
}

void lobj_set_class(lky_object *obj, lky_object *cls)
{
    obj->cls = (struct lky_object *)cls;
    lobj_set_member(obj, "class_", cls);
}

char lobj_is_of_class(lky_object *obj, void *cls)
{
    if(obj->type != LBI_CUSTOM && obj->type != LBI_CUSTOM_EX)
        return 0;

    return ((void *)obj->cls) == cls;
}

char lobj_have_same_class(lky_object *a, lky_object *b)
{
    return lobj_is_of_class(b, (void *)a->cls);
}

char *lobj_stringify(lky_object *obj)
{
    lky_object_function *func = (lky_object_function *)lobj_get_member(obj, "stringify_");
    if(!func)
        return NULL;
    
    lky_func_bundle b = MAKE_BUNDLE(func, NULL);
    lky_object *strobj = (lky_object *)(func->callable.function)(&b);
    if(!strobj)
        return NULL;

    if((void *)strobj->cls != (void *)stlstr_class())
        return NULL;

    return ((lky_object_custom *)strobj)->data;
}

void lobj_dealloc(lky_object *obj)
{
    if(obj->type != LBI_CUSTOM && obj->type != LBI_CUSTOM_EX)
        lobjb_clean(obj);
    else if(obj->type == LBI_CUSTOM_EX)
    {
        lky_object_custom *cu = (lky_object_custom *)obj;
        if(cu->freefunc)
            cu->freefunc(obj);
    }

    if(obj->type != LBI_INTEGER && obj->type != LBI_FLOAT &&
            obj->type != LBI_SEQUENCE && obj->type != LBI_CODE && obj->type != LBI_ITERABLE)
        hst_free(&obj->members);

    aqua_release(obj);
}

void print_alloced()
{
    printf(">>>> %d\n", alloced);
}
