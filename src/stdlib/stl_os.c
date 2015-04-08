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

#include "stl_os.h"
#include "stl_string.h"
#include "stl_array.h"

static lky_object *_stl_os_class_ = NULL;

lky_object *stlos_system(lky_func_bundle *b)
{
    lky_object_seq *args = BUW_ARGS(b);
    char *msg = lobjb_stringify((lky_object *)args->value, BUW_INTERP(b));
    system(msg);
    free(msg);
    return &lky_nil;
}

void stlos_init(int argc, char *argv[])
{
    _stl_os_class_ = lobj_alloc();

    arraylist list = arr_create(argc + 1);
    int i;
    for(i = 0; i < argc; i++)
        arr_append(&list, stlstr_cinit(argv[i]));

    lobj_set_member(_stl_os_class_, "argc", lobjb_build_int(argc));
    lobj_set_member(_stl_os_class_, "argv", stlarr_cinit(list));
    lobj_set_member(_stl_os_class_, "system", lobjb_build_func_ex(_stl_os_class_, 1, (lky_function_ptr)stlos_system));
}

lky_object *stlos_get_class()
{
    return _stl_os_class_;
}
