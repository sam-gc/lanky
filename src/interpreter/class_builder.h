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

#include "lky_object.h"
#include "lkyobj_builtin.h"

typedef enum {
    LCP_PROTO = 0,
    LCP_STATIC = 1,
    LCP_INIT
} lky_class_prefix;

lky_object *clb_init_class(lky_object *init_func);
void clb_add_member(lky_object *cls, char *refname, lky_object *obj, lky_class_prefix how);

#endif
