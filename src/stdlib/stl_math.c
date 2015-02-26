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

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "stl_math.h"
#include "stl_array.h"
#include "stl_units.h"

#define M_PI 3.14159265358979323846
#define M_E  2.71828182845904523536

#define TOKENPASTE(x, y) x ## y
#define IS_NUMBER(obj) (((uintptr_t)(obj) & 1) || obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)

// Here we are shooting for rough templating. Lots of the cmath functions
// take one value as input and we want to wrap all of them. The below
// macro takes the name of a function and wraps it into a function.
// Then we use the next macro to wrap all of those up into the math
// object.
#define STLMATH_WRAP_FUNC(function) \
lky_object *TOKENPASTE(stlmath_wrap_, function) (lky_func_bundle *bundle) \
{\
    lky_object_seq *args = BUW_ARGS(bundle);\
    lky_object_builtin *b = (lky_object_builtin *)args->value;\
    if(!IS_NUMBER(b))\
    {\
        return &lky_nil;\
    }\
\
    double val = OBJ_NUM_UNWRAP(b);\
\
    return lobjb_build_float( function (val));\
}

#define STLMATH_WRAP_MEMBER(obj, function) (lobj_set_member(obj, #function, lobjb_build_func_ex(obj, 1, (lky_function_ptr)TOKENPASTE(stlmath_wrap_, function))))

lky_object *stlmath_wrap_rand(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    return lobjb_build_int(rand());
}

lky_object *stlmath_shuffle(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *arrobj = (lky_object *)args->value;
    arraylist arr = stlarr_unwrap(arrobj);

    void *pts[arr.count];
    void *npts[arr.count];
    memcpy(pts, arr.items, sizeof(void *) * arr.count);
    int i, tot;
    for(i = 0, tot = arr.count; i < arr.count; i++)
    {
        int idx = rand() % tot;
        void *obj = pts[idx];
        npts[i] = obj;
        int j;
        for(j = idx; j < tot - 1; j++)
            pts[j] = pts[j + 1];
        tot--;
    }

    arraylist list;
    list.count = arr.count;
    list.allocated = arr.count + 8;
    list.items = calloc(arr.count + 8, sizeof(void *));
    memcpy(list.items, npts, sizeof(void *) * arr.count);

    return stlarr_cinit(list);
}

lky_object *stlmath_range(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *maxobj = (lky_object *)args->value;
    int max = (int)OBJ_NUM_UNWRAP(maxobj);
    arraylist list = arr_create(max + 8);
    int i;
    for(i = 0; i < max; i++)
        arr_append(&list, lobjb_build_int(i));

    return stlarr_cinit(list);
}

lky_object *stlmath_rand_int(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_builtin *first = (lky_object_builtin *)args->value;
    lky_object_builtin *second = (lky_object_builtin *)args->next->value;

    if(!IS_NUMBER(first) || !IS_NUMBER(second))
    {
        // TODO: Error
        return lobjb_build_int(0);
    }

    long bot = OBJ_NUM_UNWRAP(first);
    long top = OBJ_NUM_UNWRAP(second);

    long dif = top - bot;

    long rv = rand() % dif + bot;

    return lobjb_build_int(rv);
}

lky_object *stlmath_quad(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_builtin *first = (lky_object_builtin *)args->value;
    lky_object_builtin *second = (lky_object_builtin *)args->next->value;
    lky_object_builtin *third = (lky_object_builtin *)args->next->next->value;

    if(!IS_NUMBER(first) || !IS_NUMBER(second) || !IS_NUMBER(third))
    {
        // TODO: Error
        return lobjb_build_int(0);
    }

    double a = OBJ_NUM_UNWRAP(first);
    double b = OBJ_NUM_UNWRAP(second);
    double c = OBJ_NUM_UNWRAP(third);

    double root = sqrt(b * b - 4 * a * c);
    double num1 = -b + root;
    double num2 = -b - root;
    double den = 2 * a;

    double x1 = num1 / den;
    double x2 = num2 / den;

    arraylist list = arr_create(3);
    arr_append(&list, lobjb_build_float(x1));
    arr_append(&list, lobjb_build_float(x2));

    return stlarr_cinit(list);
}

STLMATH_WRAP_FUNC(sin)
STLMATH_WRAP_FUNC(cos)
STLMATH_WRAP_FUNC(tan)
STLMATH_WRAP_FUNC(abs)
STLMATH_WRAP_FUNC(acos)
STLMATH_WRAP_FUNC(asin)
STLMATH_WRAP_FUNC(atan)
STLMATH_WRAP_FUNC(cosh)
STLMATH_WRAP_FUNC(sinh)
STLMATH_WRAP_FUNC(tanh)
STLMATH_WRAP_FUNC(acosh)
STLMATH_WRAP_FUNC(asinh)
STLMATH_WRAP_FUNC(atanh)
STLMATH_WRAP_FUNC(exp)
STLMATH_WRAP_FUNC(log)
STLMATH_WRAP_FUNC(sqrt)
STLMATH_WRAP_FUNC(ceil)
STLMATH_WRAP_FUNC(floor)
STLMATH_WRAP_FUNC(round)

lky_object *stlmath_get_astro_class()
{
    lky_object *obj = lobj_alloc();    

    lobj_set_member(obj, "gravEarth", stlun_cinit(9.8, "m/s^2"));
    lobj_set_member(obj, "massEarth", stlun_cinit(5.975e24, "kg"));
    lobj_set_member(obj, "radiusEarth", stlun_cinit(6378, "km"));
    lobj_set_member(obj, "massSun", stlun_cinit(1.989e30, "kg"));
    lobj_set_member(obj, "radiusSun", stlun_cinit(6.96e8, "m"));
    lobj_set_member(obj, "lumSun", stlun_cinit(3.847e26, "kg*m^2/s^3"));
    lobj_set_member(obj, "sbc", stlun_cinit(5.6705, "kg"));

    return obj;
}

lky_object *stlmath_get_phys_class()
{
    lky_object *obj = lobj_alloc();

    lobj_set_member(obj, "c", stlun_cinit(2.9979e8, "m/s"));
    lobj_set_member(obj, "G", stlun_cinit(6.6726e-11, "m^3/kg*s^2"));
    lobj_set_member(obj, "h", stlun_cinit(6.6261e-34, "kg*m^2/s"));
    lobj_set_member(obj, "me", stlun_cinit(9.1094e-31, "g"));
    lobj_set_member(obj, "mH", stlun_cinit(1.6735e-24, "g"));

    return obj;
}

lky_object *stlmath_get_class()
{
    srand((unsigned int)time(NULL));
    lky_object *obj = lobj_alloc();
    lobj_set_member(obj, "Astro", stlmath_get_astro_class());
    lobj_set_member(obj, "Phys", stlmath_get_phys_class());
    lobj_set_member(obj, "rand", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlmath_wrap_rand));
    lobj_set_member(obj, "randInt", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlmath_rand_int));
    lobj_set_member(obj, "quad", lobjb_build_func_ex(obj, 3, (lky_function_ptr)stlmath_quad));
    lobj_set_member(obj, "shuffle", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlmath_shuffle));
    lobj_set_member(obj, "range", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlmath_range));
    STLMATH_WRAP_MEMBER(obj, sin);
    STLMATH_WRAP_MEMBER(obj, cos);
    STLMATH_WRAP_MEMBER(obj, tan);
    STLMATH_WRAP_MEMBER(obj, abs);
    STLMATH_WRAP_MEMBER(obj, acos);
    STLMATH_WRAP_MEMBER(obj, asin);
    STLMATH_WRAP_MEMBER(obj, atan);
    STLMATH_WRAP_MEMBER(obj, cosh);
    STLMATH_WRAP_MEMBER(obj, sinh);
    STLMATH_WRAP_MEMBER(obj, tanh);
    STLMATH_WRAP_MEMBER(obj, acosh);
    STLMATH_WRAP_MEMBER(obj, asinh);
    STLMATH_WRAP_MEMBER(obj, atanh);
    STLMATH_WRAP_MEMBER(obj, exp);
    STLMATH_WRAP_MEMBER(obj, log);
    STLMATH_WRAP_MEMBER(obj, sqrt);
    STLMATH_WRAP_MEMBER(obj, ceil);
    STLMATH_WRAP_MEMBER(obj, floor);
    STLMATH_WRAP_MEMBER(obj, round);

    lobj_set_member(obj, "pi", lobjb_build_float(M_PI));
    lobj_set_member(obj, "e", lobjb_build_float(M_E));
    return obj;
}
