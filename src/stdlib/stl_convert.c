#include <string.h>
#include "stl_convert.h"

lky_object *stlcon_to_int(lky_object_seq *args, lky_object *func)
{
    lky_object *from = args->value;

    if(from->type == LBI_INTEGER)
        return from;

    lky_object_builtin *b = (lky_object_builtin *)from;
    if(from->type == LBI_FLOAT)
        return(lobjb_build_int((long)b->value.d));

    if(from->type != LBI_STRING)
        return &lky_nil;

    long val;
    sscanf(b->value.s, "%ld", &val);

    return lobjb_build_int(val);
}

lky_object *stlcon_get_class()
{
    lky_object *obj = lobj_alloc();

    lobj_set_member(obj, "toInt", lobjb_build_func_ex(obj, 1, stlcon_to_int));

    return obj;
}
