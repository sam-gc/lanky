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

lky_object *clb_new_wrapper(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *cls = (lky_object *)func->owner;

    lky_object *nobj = lobj_alloc();
    lobj_set_member(nobj, "proto_", lobj_get_member(cls, "model_"));

    lky_object_seq *nx = lobjb_make_seq_node(nobj);
    nx->next = args;

    lobjb_call(lobj_get_member(cls, "init_"), nx);

    return nobj;
}

lky_object *clb_init_class(lky_object *init_func)
{
    lky_object_function *fobj = (lky_object_function *)init_func;
    lky_object *cls = lobj_alloc();
    lobj_set_member(cls, "init_", init_func);
    lobj_set_member(cls, "new", lobjb_build_func_ex(cls, fobj->callable.argc - 1, (lky_function_ptr)clb_new_wrapper));
    lobj_set_member(cls, "model_", lobj_alloc());

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
    }
}
