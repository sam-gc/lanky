#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "stl_math.h"

lky_object *stlmath_wrap_rand(lky_object_seq *args, lky_object *func)
{
    return lobjb_build_int(rand());
}

lky_object *stlmath_get_class()
{
    srand((unsigned int)time(NULL));
    lky_object *obj = lobj_alloc();
    lobj_set_member(obj, "rand", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlmath_wrap_rand));
    return obj;
}
