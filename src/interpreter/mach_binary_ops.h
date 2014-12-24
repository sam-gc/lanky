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

#ifndef MACH_BINARY_OPS
#define MACH_BINARY_OPS

#include "lkyobj_builtin.h"

lky_object *lobjb_binary_add(lky_object *a, lky_object *b);
lky_object *lobjb_binary_subtract(lky_object *a, lky_object *b);
lky_object *lobjb_binary_multiply(lky_object *a, lky_object *b);
lky_object *lobjb_binary_divide(lky_object *a, lky_object *b);
lky_object *lobjb_binary_modulo(lky_object *a, lky_object *b);
lky_object *lobjb_binary_power(lky_object *a, lky_object *b);
lky_object *lobjb_binary_lessthan(lky_object *a, lky_object *b);
lky_object *lobjb_binary_greaterthan(lky_object *a, lky_object *b);
lky_object *lobjb_binary_equals(lky_object *a, lky_object *b);
lky_object *lobjb_binary_lessequal(lky_object *a, lky_object *b);
lky_object *lobjb_binary_greatequal(lky_object *a, lky_object *b);
lky_object *lobjb_binary_notequal(lky_object *a, lky_object *b);
lky_object *lobjb_binary_and(lky_object *a, lky_object *b);
lky_object *lobjb_binary_or(lky_object *a, lky_object *b);

#endif
