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
#include "stl_table.h"
#include "class_builder.h"
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

time_data *stltime_make_blob(lky_object *ht)
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

    time_data *data = malloc(sizeof(time_data));
    data->time = td;

    return data;
    /*cobj->data = data;

    lky_object *obj = (lky_object *)cobj;

    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stltime_date_stringify));
    lobj_set_member(obj, "format", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltime_date_format));

    cobj->freefunc = stltime_date_free;

    stltime_copy_props_from_struct(obj);

    return obj;*/
}

void stltime_copy_props_from_struct(lky_object *o)
{
    time_data *data = CLASS_GET_BLOB(o, "db_", time_data *);
    struct tm tm = data->time;
    
    SET_INT_MEMBER(o, "second", tm.tm_sec);   
    SET_INT_MEMBER(o, "minute", tm.tm_min);
    SET_INT_MEMBER(o, "hour", tm.tm_hour);
    SET_INT_MEMBER(o, "day", tm.tm_mday);
    SET_INT_MEMBER(o, "month", tm.tm_mon);
    SET_INT_MEMBER(o, "year", tm.tm_year + 1900);
    SET_INT_MEMBER(o, "dayOfWeek", tm.tm_wday);
    SET_INT_MEMBER(o, "dayOfYear", tm.tm_yday);
#ifdef _GNU_SOURCE
    lobj_set_member(o, "timeZone", stlstr_cinit((char *)tm.tm_zone));
    SET_INT_MEMBER(o, "offset", tm.tm_gmtoff);
#else
    lobj_set_member(o, "timeZone", &lky_nil);
    lobj_set_member(o, "offset", &lky_nil);
#endif
}

void stltime_copy_props_to_struct(lky_object *obj)
{
    time_data *data = CLASS_GET_BLOB(obj, "db_", time_data *);
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

CLASS_MAKE_BLOB_FUNCTION(stltime_on_gc, time_data *, data, how,
    if(how == CGC_FREE)
        free(data);    
)

CLASS_MAKE_INIT(stltime_init,
    time_data *blob = stltime_make_blob($1);
    CLASS_SET_BLOB(self_, "db_", blob, stltime_on_gc);
    stltime_copy_props_from_struct(self_);
)

CLASS_MAKE_METHOD_EX(stltime_stringify, self, time_data *, db_, 
    if(!db_)
        return stlstr_cinit("(Time | Incomplete Object)");

    time_data *d = db_;
    struct tm td = d->time;
    char buf[500];
    asctime_r(&td, buf + 8);
    memcpy(buf, "(Time | ", 8);
    buf[strlen(buf) - 1] = ')';

    return stlstr_cinit(buf);
)

CLASS_MAKE_METHOD_EX(stltime_format, self, time_data *, db_,
    stltime_copy_props_to_struct(self);
    char *str = lobjb_stringify($1, interp_);

    char buf[1000];
    time_data *data = CLASS_GET_BLOB(self, "db_", time_data *);
    struct tm tm = data->time;
    strftime(buf, 1000, str, &tm);

    free(str);
    return stlstr_cinit(buf);
)

CLASS_MAKE_METHOD(stltime_unix, self,
    return lobjb_build_int(millis());
)

static lky_object *stltime_class_;
lky_object *stltime_get_class()
{
    if(stltime_class_)
        return stltime_class_;

    CLASS_MAKE(cls, NULL, stltime_init, 1,
        CLASS_STATIC_METHOD("unix", stltime_unix, 0);
        CLASS_PROTO_METHOD("format", stltime_format, 1);
        CLASS_PROTO_METHOD("stringify_", stltime_stringify, 0);
    );

    stltime_class_ = cls;

    return cls;
}
