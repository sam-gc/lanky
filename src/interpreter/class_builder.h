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

typedef void (*clb_custom_init_func)(lky_object *obj, lky_object *cls, void *data);

lky_object *clb_init_class(lky_object *init_func, lky_object *super);
lky_object *clb_instantiate(lky_object *cls, clb_custom_init_func, void *data);
void clb_add_member(lky_object *cls, char *refname, lky_object *obj, lky_class_prefix how);

#ifdef __GNUC__
#define ATTRIB_NO_USE __attribute__((unused))
#else
#define ATTRIB_NO_USE
#endif

#define CLASS_MAKE(name, super, init, argc, code)\
    lky_object *init_func_ = (lky_object *)lobjb_build_func_ex(NULL, argc, (lky_function_ptr)init);\
    lky_object *cls_ = clb_init_class(init_func_, super);\
    code\
    lky_object *name = cls_

#define CLASS_PROTO(name, obj) clb_add_member(cls_, name, obj, LCP_PROTO)
#define CLASS_STATIC(name, obj) clb_add_member(cls_, name, obj, LCP_STATIC)
#define CLASS_PROTO_METHOD(name, ptr, argc) CLASS_PROTO(name, (lky_object *)lobjb_build_func_ex(NULL, argc, (lky_function_ptr)ptr))
#define CLASS_STATIC_METHOD(name, ptr, argc) CLASS_STATIC(name, (lky_object *)lobjb_build_func_ex(NULL, argc, (lky_function_ptr)ptr))
#define CLASS_MAKE_METHOD(name, ident, code...) lky_object * name (lky_func_bundle *bundle_) {\
    lky_object_seq *args_ ATTRIB_NO_USE = BUW_ARGS(bundle_);\
    lky_object_function *func_ ATTRIB_NO_USE = BUW_FUNC(bundle_);\
    mach_interp *interp_ ATTRIB_NO_USE = BUW_INTERP(bundle_);\
    lky_object * ident ATTRIB_NO_USE = func_->bound;\
    lky_object *$1 ATTRIB_NO_USE = args_ ? (lky_object *)args_->value : NULL;\
    lky_object *$2 ATTRIB_NO_USE = args_ && args_->next ? (lky_object *)args_->next->value : NULL;\
    lky_object *$3 ATTRIB_NO_USE = args_ && args_->next && args_->next->next ? (lky_object *) args_->next->next->value : NULL;\
    code\
    return &lky_nil;}
#define CLASS_MAKE_METHOD_EX(name, ident, type, key, code...) lky_object * name (lky_func_bundle *bundle_) {\
    lky_object_seq *args_ ATTRIB_NO_USE = BUW_ARGS(bundle_);\
    lky_object_function *func_ ATTRIB_NO_USE = BUW_FUNC(bundle_);\
    mach_interp *interp_ ATTRIB_NO_USE = BUW_INTERP(bundle_);\
    lky_object * ident ATTRIB_NO_USE = func_->bound;\
    lky_object *$1 ATTRIB_NO_USE = args_ ? (lky_object *)args_->value : NULL;\
    lky_object *$2 ATTRIB_NO_USE = args_ && args_->next ? (lky_object *)args_->next->value : NULL;\
    lky_object *$3 ATTRIB_NO_USE = args_ && args_->next && args_->next->next ? (lky_object *) args_->next->next->value : NULL;\
    type key ATTRIB_NO_USE = (type) ((lky_object_builtin *)lobj_get_member(ident, #key ))->value.b;\
    code\
    return &lky_nil;}

#define CLASS_SET_BLOB(obj, key, ptr, gc) (lobj_set_member(obj, key, lobjb_build_blob(ptr, (lobjb_void_ptr_function)gc)))

#define CLASS_MAKE_INIT(name, code...) lky_object * name (lky_func_bundle *bundle_) {\
    lky_object_seq *args_ ATTRIB_NO_USE = BUW_ARGS(bundle_);\
    lky_object_function *func_ ATTRIB_NO_USE = BUW_FUNC(bundle_);\
    mach_interp *interp_ ATTRIB_NO_USE = BUW_INTERP(bundle_);\
    lky_object *self_ ATTRIB_NO_USE = (lky_object *)args_->value;\
    lky_object *$1 ATTRIB_NO_USE = args_ && args_->next ? (lky_object *)args_->next->value : NULL;\
    lky_object *$2 ATTRIB_NO_USE = args_ && args_->next && args_->next->next ? (lky_object *) args_->next->next->value : NULL;\
    lky_object *$3 ATTRIB_NO_USE = args_ && args_->next && args_->next->next && args_->next->next->next ? (lky_object *) args_->next->next->next->value : NULL;\
    code\
    return &lky_nil;}

#define CLASS_MAKE_BLOB_FUNCTION(name, type, ident, how, code...) void name (void *obj_, lky_class_gc_type how ) {\
    type ident = (type)obj_; code}

#define CLASS_GET_BLOB(obj, key, type) ((type) ((lky_object_builtin *)lobj_get_member(obj, key))->value.b)

#endif
