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

#include "stl_time.h"
#include <sys/timeb.h>

long millis() 
{
    struct timeb tb;
    ftime(&tb);
    return (long)(tb.time * 1000 + tb.millitm);
}

lky_object *stltime_millis(lky_object_seq *args, lky_object *func)
{
    return lobjb_build_int(millis());
}

lky_object *stltime_get_class()
{
    lky_object *obj = lobj_alloc();
    lobj_set_member(obj, "unix", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stltime_millis));

    return obj;
}
