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

#include "Lanky.h"

lky_object *test2_print_message(lky_object_seq *args, lky_object *func)
{
    lky_object *o = (lky_object *)args->value;

    lobjb_print_object(o);
    printf("%lf\n", OBJ_NUM_UNWRAP(o));

    return lobjb_build_int(1);
}

lky_object *test2_init()
{
    return lobjb_build_func_ex(NULL, 1, (lky_function_ptr)test2_print_message);
}
