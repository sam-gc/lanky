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

#include <string.h>
#include "stl_convert.h"
#include "stl_units.h"
#include "stl_string.h"
#include "stl_array.h"

#define IS_TAGGED(a) ((uintptr_t)(a) & 1)

lky_object *stlcon_to_int(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *from = (lky_object *)args->value;

    if(IS_TAGGED(from) || from->type == LBI_INTEGER)
        return from;
    if(from->type == LBI_FLOAT)
        return lobjb_build_int(OBJ_NUM_UNWRAP(from));

    lky_object_custom *b = (lky_object_custom *)from;
//    if(from->type == LBI_FLOAT)
//        return(lobjb_build_int((long)b->value.d));

//    if(from->type != LBI_STRING)
//        return &lky_nil;

    long val;
    sscanf(b->data, "%ld", &val);

    return lobjb_build_int(val);
}

lky_object *stlcon_to_float(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *from = (lky_object *)args->value;

    if(IS_TAGGED(from) || from->type == LBI_INTEGER)
        return lobjb_build_float(OBJ_NUM_UNWRAP(from));

    if(from->type == LBI_FLOAT)
        return from;

    lky_object_custom *b = (lky_object_custom *)from;

    double val;
    sscanf(b->data, "%lf", &val);

    return lobjb_build_float(val);
}

lky_object *stlcon_to_string(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *from = (lky_object *)args->value;
    char *str = lobjb_stringify(from, BUW_INTERP(bundle));
    lky_object *ret = stlstr_cinit(str);
    free(str);
    return ret;
}

lky_object *stlcon_ord(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *from = (lky_object *)args->value;
    char *str = lobjb_stringify(from, BUW_INTERP(bundle));
    lky_object *ret = NULL;

    size_t len = strlen(str);
    if(len == 1)
        ret = lobjb_build_int(str[0]);
    else
    {
        arraylist list = arr_create(len + 1);
        char *tmp = str;
        while(*tmp)
            arr_append(&list, lobjb_build_int(*tmp++));

        ret = stlarr_cinit(list);
    }

    free(str);

    return ret;
}

lky_object *stlcon_char(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *from = (lky_object *)args->value;
    if(!OBJ_IS_INTEGER(from))
        return &lky_nil;

    char c[2];
    c[0] = (char)OBJ_NUM_UNWRAP(from);
    c[1] = '\0';

    return stlstr_cinit(c);
}

lky_object *stlcon_unit(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *c = (lky_object_custom *)args->value;
    lky_object *to = (lky_object *)args->next->value;

    char *str = lobjb_stringify(to, BUW_INTERP(bundle));
    un_unit t = un_create(0, str);

    stlun_data *data = c->data;
    lky_object *ret = stlun_cinit_ex(un_add(t, data->u));
    free(str);
    return ret;
}

lky_object *stlcon_get_class()
{
    lky_object *obj = lobj_alloc();

    lobj_set_member(obj, "toInt", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlcon_to_int));
    lobj_set_member(obj, "toFloat", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlcon_to_float));
    lobj_set_member(obj, "toString", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlcon_to_string));
    lobj_set_member(obj, "toOrd", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlcon_ord));
    lobj_set_member(obj, "toChar", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlcon_char));
    lobj_set_member(obj, "Unit", stlun_get_class());
    lobj_set_member(obj, "units", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlcon_unit));

    return obj;
}
