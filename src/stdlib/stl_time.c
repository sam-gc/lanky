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
#include "hashtable.h"
#include "stl_string.h"
#include <sys/timeb.h>
#include <string.h>
#include <time.h>

#define SET_INT_MEMBER(obj, name, val) (lobj_set_member((lky_object *)obj, name, (lky_object *)lobjb_build_int(val)))
#define APPLY_INT_MEMBER(obj, name, targ) (targ = (int)(OBJ_NUM_UNWRAP((lobj_get_member((lky_object *)obj, name)))))
#define SET_FROM_DICT(dict, name, targ) do {\
    lky_object *obj = stltab_cget(dict, stlstr_cinit(name));\
    if(obj)\
        targ = (int)(OBJ_NUM_UNWRAP(obj));\
} while(0)

typedef struct {
    struct tm time;
} time_data;

long millis() 
{
    struct timeb tb;
    ftime(&tb);
    return (long)(tb.time * 1000 + tb.millitm);
}

lky_object *stltime_millis(lky_func_bundle *bundle)
{
    return lobjb_build_int(millis());
}

lky_object *stltime_date_stringify(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

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
#ifdef _GNU_SOURCE
    lobj_set_member((lky_object *)obj, "timeZone", stlstr_cinit((char *)tm.tm_zone));
    SET_INT_MEMBER((lky_object *)obj, "offset", tm.tm_gmtoff);
#else
    lobj_set_member((lky_object *)obj, "timeZone", &lky_nil);
    lobj_set_member((lky_object *)obj, "offset", &lky_nil);
#endif
}

void stltime_copy_props_to_struct(lky_object *o)
{
    lky_object_custom *obj = (lky_object_custom *)o;
    time_data *data = obj->data;
    struct tm tm = data->time;

    APPLY_INT_MEMBER(obj, "second", tm.tm_sec);
    APPLY_INT_MEMBER(obj, "minute", tm.tm_min);
    APPLY_INT_MEMBER(obj, "hour", tm.tm_hour);
    APPLY_INT_MEMBER(obj, "day", tm.tm_mday);
    APPLY_INT_MEMBER(obj, "month", tm.tm_mon);
    APPLY_INT_MEMBER(obj, "year", tm.tm_year);
    APPLY_INT_MEMBER(obj, "dayOfWeek", tm.tm_wday);
    APPLY_INT_MEMBER(obj, "dayOfYear", tm.tm_yday);
    tm.tm_year -= 1900;

    data->time = tm;
}

lky_object *stltime_date_format(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stltime_copy_props_to_struct((lky_object *)self);
    char *str = lobjb_stringify((lky_object *)args->value, BUW_INTERP(bundle));

    char buf[1000];
    time_data *data = self->data;
    struct tm tm = data->time;
    strftime(buf, 1000, str, &tm);

    free(str);
    return stlstr_cinit(buf);
}

lky_object *stltime_build_date_object(lky_object *ht)
{
    struct tm td;
    time_t rt;
    time(&rt);
    td = *localtime(&rt);
    if(ht)
    {
        SET_FROM_DICT(ht, "second", td.tm_sec);
        SET_FROM_DICT(ht, "minute", td.tm_min);
        SET_FROM_DICT(ht, "hour", td.tm_hour);
        SET_FROM_DICT(ht, "day", td.tm_mday);
        SET_FROM_DICT(ht, "month", td.tm_mon);
        SET_FROM_DICT(ht, "dayOfWeek", td.tm_wday);
        SET_FROM_DICT(ht, "dayOfYear", td.tm_yday);
        
        lky_object *yobj = stltab_cget(ht, stlstr_cinit("year"));
        if(yobj)
            td.tm_year = (int)(OBJ_NUM_UNWRAP(yobj)) - 1900;
    }

    lky_object_custom *cobj = lobjb_build_custom(sizeof(time_data));
    time_data *data = malloc(sizeof(time_data));
    data->time = td;
    cobj->data = data;

    lky_object *obj = (lky_object *)cobj;

    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stltime_date_stringify));
    lobj_set_member(obj, "format", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltime_date_format));

    cobj->freefunc = stltime_date_free;

    stltime_copy_props_from_struct(obj);

    return obj;
}

lky_object *stltime_date_builder(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *indict = args ? (lky_object *)args->value : NULL;
    return stltime_build_date_object(indict);
}

lky_object *stltime_get_class()
{
    lky_object *obj = lobj_alloc();
    lobj_set_member(obj, "unix", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stltime_millis));
    lobj_set_member(obj, "create", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltime_date_builder));

    return obj;
}
