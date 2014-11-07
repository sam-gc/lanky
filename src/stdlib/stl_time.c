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
