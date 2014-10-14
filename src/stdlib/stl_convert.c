#include <string.h>
#include "stl_convert.h"
#include "stl_units.h"
#include "stl_string.h"

lky_object *stlcon_to_int(lky_object_seq *args, lky_object *func)
{
    lky_object *from = (lky_object *)args->value;

    if(from->type == LBI_INTEGER)
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

lky_object *stlcon_to_float(lky_object_seq *args, lky_object *func)
{
    lky_object *from = (lky_object *)args->value;

    if(from->type == LBI_FLOAT)
        return from;
    if(from->type == LBI_INTEGER)
        return lobjb_build_float(OBJ_NUM_UNWRAP(from));

    lky_object_custom *b = (lky_object_custom *)from;

    double val;
    sscanf(b->data, "%lf", &val);

    return lobjb_build_float(val);
}

lky_object *stlcon_to_string(lky_object_seq *args, lky_object *func)
{
    lky_object *from = (lky_object *)args->value;
    char *str = lobjb_stringify(from);
    lky_object *ret = stlstr_cinit(str);
    free(str);
    return ret;
}

lky_object *stlcon_unit(lky_object_seq *args, lky_object *func)
{
    lky_object_custom *c = (lky_object_custom *)args->value;
    lky_object *to = (lky_object *)args->next->value;

    char *str = lobjb_stringify(to);
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
    lobj_set_member(obj, "Unit", stlun_get_class());
    lobj_set_member(obj, "units", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlcon_unit));

    return obj;
}
