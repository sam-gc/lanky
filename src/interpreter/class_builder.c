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

#include "class_builder.h"

lky_object *clb_super_init_wrapper(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *self = (lky_object *)func->bound;

    lky_object *super = lobj_get_member((lky_object *)func->owner, "super_");
    lky_object *init = lobj_get_member(super, "init_");
    if(init)
    {
        lky_object *sinit = lobj_get_member(super, "super_init_");
        //lobjb_print(super);
        if(sinit)
            lobj_set_member(self, "superInit", sinit);

        lky_object_seq *nx = lobjb_make_seq_node(self);
        nx->next = args;
        lobjb_call(init, nx, BUW_INTERP(bundle));
    }

    return &lky_nil;
}

lky_object *clb_new_wrapper(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *cls = (lky_object *)func->owner;

    lky_object *nobj = lobj_alloc();
    lobj_set_member(nobj, "proto_", lobj_get_member(cls, "model_"));
    lobj_set_member(nobj, "class_", cls);

    lky_object *init = lobj_get_member(cls, "init_");
    if(init)
    {
        lky_object *sinit = lobj_get_member(cls, "super_init_");
        if(sinit)
            lobj_set_member(nobj, "superInit", sinit);

        lky_object_seq *nx = lobjb_make_seq_node(nobj);
        nx->next = args;

        lobjb_call(init, nx, BUW_INTERP(bundle));
    }

    return nobj;
}

lky_object *clb_instantiate(lky_object *cls, clb_custom_init_func func, void *data)
{
    lky_object *nobj = lobj_alloc();
    lobj_set_member(nobj, "proto_", lobj_get_member(cls, "model_"));
    lobj_set_member(nobj, "class_", cls);

    if(func)
        func(nobj, cls, data);
    else
    {
        lky_object *init = lobj_get_member(cls, "init_");
        if(init)
        {
            lky_object *sinit = lobj_get_member(cls, "super_init_");
            if(sinit)
                lobj_set_member(nobj, "superInit", sinit);

            lky_object_seq *nx = lobjb_make_seq_node(nobj);
            lobjb_call(init, nx, NULL);
        }
    } 

    return nobj;
}

lky_object *clb_init_class(lky_object *init_func, lky_object *super)
{
    lky_object_function *fobj = (lky_object_function *)init_func;
    lky_object *cls = lobj_alloc();

    int argc = 0;
    if(init_func)
    {
        lobj_set_member(cls, "init_", init_func);
        argc = fobj->callable.argc - 1;
    }

    if(super)
    {
        lobj_set_member(cls, "super_", super);
        lky_object_function *sinit = (lky_object_function *)lobj_get_member(super, "init_");
        int argc = sinit ? sinit->callable.argc - 1 : 0;
        lobj_set_member(cls, "super_init_", lobjb_build_func_ex(cls, argc, (lky_function_ptr)clb_super_init_wrapper));
    }

    lobj_set_member(cls, "new", lobjb_build_func_ex(cls, argc, (lky_function_ptr)clb_new_wrapper));

    lky_object *model = lobj_alloc();
    if(super)
        lobj_set_member(model, "proto_", lobj_get_member(super, "model_"));
    lobj_set_member(cls, "model_", model);

    return cls;
}


void clb_add_member(lky_object *cls, char *refname, lky_object *obj, lky_class_prefix how)
{
    switch(how)
    {
        case LCP_PROTO:
        {
            lky_object *mod = lobj_get_member(cls, "model_");
            lobj_set_member(mod, refname, obj);
        }
        break;
        case LCP_STATIC:
            lobj_set_member(cls, refname, obj);
        break;
        default:
        break;
    }
}
