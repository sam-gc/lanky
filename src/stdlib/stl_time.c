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

#define _POSIX_SOURCE
#include "stl_time.h"
#include "hashtable.h"
#include "stl_string.h"
#include <sys/timeb.h>
#include <string.h>
#include <time.h>

#define SET_INT_MEMBER(obj, name, val) (lobj_set_member((lky_object *)obj, name, (lky_object *)lobjb_build_int(val)))

typedef struct {
    struct tm time;
} time_data;

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

lky_object *stltime_date_stringify(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    time_data *d = self->data;
    struct tm td = d->time;
    char buf[500];
    asctime_r(&td, buf + 7);
    memcpy(buf, "(Time: ", 7);
    buf[strlen(buf) - 1] = ')';

    return stlstr_cinit(buf);
}

void stltime_date_free(lky_object *o)
{
    lky_object_custom *obj = (lky_object_custom *)o;
    free(obj->data);
}

void stltime_copy_props_from_struct(lky_object *o)
{
    lky_object_custom *obj = (lky_object_custom *)o;
    time_data *data = obj->data;
    struct tm tm = data->time;
    
    SET_INT_MEMBER(obj, "second", tm.tm_sec);   
    SET_INT_MEMBER(obj, "minute", tm.tm_min);
    SET_INT_MEMBER(obj, "hour", tm.tm_hour);
    SET_INT_MEMBER(obj, "day", tm.tm_mday);
    SET_INT_MEMBER(obj, "month", tm.tm_mon);
    SET_INT_MEMBER(obj, "year", tm.tm_year + 1900);
    SET_INT_MEMBER(obj, "dayOfWeek", tm.tm_wday);
    SET_INT_MEMBER(obj, "dayOfYear", tm.tm_yday);
}

lky_object *stltime_build_date_object(hashtable *ht)
{
    struct tm *td;
    if(!ht)
    {
        time_t rt;
        time(&rt);
        td = localtime(&rt);
    }

    lky_object_custom *cobj = lobjb_build_custom(sizeof(time_data));
    time_data *data = malloc(sizeof(time_data));
    data->time = *td;
    cobj->data = data;

    lky_object *obj = (lky_object *)cobj;

    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stltime_date_stringify));
    cobj->freefunc = stltime_date_free;

    stltime_copy_props_from_struct(obj);

    return obj;
}

lky_object *stltime_date_builder(lky_object_seq *args, lky_object *func)
{
    return stltime_build_date_object(NULL);
}

lky_object *stltime_get_class()
{
    lky_object *obj = lobj_alloc();
    lobj_set_member(obj, "unix", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stltime_millis));
    lobj_set_member(obj, "create", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stltime_date_builder));

    return obj;
}
