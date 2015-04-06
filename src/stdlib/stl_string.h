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

#ifndef STL_STRING_H
#define STL_STRING_H

#include "lkyobj_builtin.h"

void stlstr_blob_func(void *data, lky_class_gc_type how);
lky_object *stlstr_cinit(char *str);
//lky_object *stlstr_fmt_ext(char *mestr, arraylist list);
//lky_object *stltab_cget(lky_object *table, lky_object *key);
//void stltab_cput(lky_object *table, lky_object *key, lky_object *val);
lky_object *stlstr_get_class();
char *stlstr_unwrap(lky_object *o);

//lky_object *stlstr_get_class();

#endif
