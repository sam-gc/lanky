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

#include "stanky.h"
#include "stl_array.h"
#include "stl_math.h"
#include "stl_io.h"
#include "stl_convert.h"
#include "stl_requisitions.h"
#include "stl_object.h"
#include "stl_time.h"
#include "stl_os.h"
#include "stl_table.h"
#include "testnew.h"
#include "lky_gc.h"
#include "lkyobj_builtin.h"

hashtable get_stdlib_objects()
{
    hashtable t = hst_create();
    t.duplicate_keys = 1;
    hst_put(&t, "Array", stlarr_get_class(), NULL, NULL);
    hst_put(&t, "Time", stltime_get_class(), NULL, NULL);
    hst_put(&t, "Math", stlmath_get_class(), NULL, NULL);
    hst_put(&t, "Io", stlio_get_class(), NULL, NULL);
    hst_put(&t, "Convert", stlcon_get_class(), NULL, NULL);
    hst_put(&t, "C", stlreq_get_class(), NULL, NULL);
    hst_put(&t, "Object", stlobj_get_class(), NULL, NULL);
    hst_put(&t, "OS", stlos_get_class(), NULL, NULL);
    hst_put(&t, "Table", stltab_get_class(), NULL, NULL);
    hst_put(&t, "Error", lobjb_get_exception_class(), NULL, NULL);
    hst_put(&t, "TN", tn_get_class(), NULL, NULL);
    return t;
}

void register_stdlib_prototypes()
{
    gc_add_object(stlarr_get_proto());
    gc_add_object(stlobj_get_proto());
}

