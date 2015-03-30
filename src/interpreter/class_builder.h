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

#ifndef CLASS_BUILDER_H
#define CLASS_BUILDER_H

#include <stdlib.h>
#include "lky_object.h"
#include "lkyobj_builtin.h"

typedef enum {
    LCP_PROTO = 0,
    LCP_STATIC = 1,
    LCP_INIT
} lky_class_prefix;

lky_object *clb_init_class(lky_object *init_func, lky_object *super);
void clb_add_member(lky_object *cls, char *refname, lky_object *obj, lky_class_prefix how);

#define CLASS_MAKE(name, super, init, argc, code)\
    lky_object *init_func_ = (lky_object *)lobjb_build_func_ex(NULL, argc, (lky_function_ptr)init);\
    lky_object *cls_ = clb_init_class(init_func_, super);\
    do code while(0);\
    lky_object *name = cls_

#define CLASS_PROTO(name, obj) clb_add_member(cls_, name, obj, LCP_PROTO)
#define CLASS_STATIC(name, obj) clb_add_member(cls_, name, obj, LCP_STATIC)
#define CLASS_PROTO_METHOD(name, ptr, argc) CLASS_PROTO(name, (lky_object *)lobjb_build_func_ex(NULL, argc, (lky_function_ptr)ptr))
#define CLASS_STATIC_METHOD(name, ptr, argc) CLASS_STATIC(name, (lky_object *)lobjb_build_func_ex(NULL, argc, (lky_function_ptr)ptr))
#define CLASS_MAKE_PROTO_METHOD(name, code) lky_object * name (lky_func_bundle *bundle_) {\
    lky_object_seq *args_ = BUW_ARGS(bundle_);\
    lky_object_function *func_ = BUW_FUNC(bundle_);\
    mach_interp *interp_ = BUW_INTERP(bundle_);\
    lky_object *self_ = func_->bound;\
    do code while(0);\
    return &lky_nil;}
#endif
