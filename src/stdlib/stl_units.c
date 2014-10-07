#include "stl_units.h"
#include "stl_string.h"

typedef struct {
    un_unit u;
} stlun_data;

lky_object *stlun_stringify(lky_object_seq *args, lky_object_function *caller)
{
    lky_object_custom *self = (lky_object_custom *)caller->owner;
    stlun_data *data = self->data;

    char buf[500];
    un_stringify(data->u, buf);

    return stlstr_cinit(buf);
}

lky_object *stlun_add(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlun_data *data = self->data;

    un_unit a = data->u;

    lky_object_custom *other = (lky_object_custom *)args->value;
    if(other->type == LBI_FLOAT || other->type == LBI_INTEGER)
    {
        a.val += OBJ_NUM_UNWRAP(other);
        return stlun_cinit_ex(a);
    }

    stlun_data *od = other->data;
    un_unit b = od->u;

    return stlun_cinit_ex(un_add(a, b));
}

lky_object *stlun_subtract(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlun_data *data = self->data;

    un_unit a = data->u;

    lky_object_custom *other = (lky_object_custom *)args->value;
    if(other->type == LBI_FLOAT || other->type == LBI_INTEGER)
    {
        a.val -= OBJ_NUM_UNWRAP(other);
        return stlun_cinit_ex(a);
    }

    stlun_data *od = other->data;
    un_unit b = od->u;

    return stlun_cinit_ex(un_sub(a, b));
}

lky_object *stlun_multiply(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlun_data *data = self->data;

    un_unit a = data->u;

    lky_object_custom *other = (lky_object_custom *)args->value;
    if(other->type == LBI_FLOAT || other->type == LBI_INTEGER)
    {
        a.val *= OBJ_NUM_UNWRAP(other);
        return stlun_cinit_ex(a);
    }

    stlun_data *od = other->data;
    un_unit b = od->u;

    return stlun_cinit_ex(un_mult(a, b));
}

lky_object *stlun_divide(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlun_data *data = self->data;

    un_unit a = data->u;

    lky_object_custom *other = (lky_object_custom *)args->value;
    if(other->type == LBI_FLOAT || other->type == LBI_INTEGER)
    {
        a.val /= OBJ_NUM_UNWRAP(other);
        return stlun_cinit_ex(a);
    }

    stlun_data *od = other->data;
    un_unit b = od->u;

    return stlun_cinit_ex(un_div(a, b));
}

void stlun_dealloc(lky_object_custom *self)
{
    free(self->data);
}

lky_object *stlun_cinit_ex(un_unit u)
{
    lky_object_custom *self = lobjb_build_custom(sizeof(stlun_data));
    lky_object *obj = (lky_object *)self;

    stlun_data *data = malloc(sizeof(stlun_data));
    data->u = u;
    self->data = data;

    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlun_stringify));
    lobj_set_member(obj, "op_add_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlun_add));
    lobj_set_member(obj, "op_subtract_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlun_subtract));
    lobj_set_member(obj, "op_multiply_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlun_multiply));
    lobj_set_member(obj, "op_divide_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlun_divide));

    self->freefunc = stlun_dealloc;

    return (lky_object *)self;
}

lky_object *stlun_cinit(double val, char *fmt)
{
    return stlun_cinit_ex(un_create(val, fmt));
}

lky_object *stlun_build(lky_object_seq *args, lky_object_function *caller)
{
    lky_object *numobj = (lky_object *)args->value;
    lky_object_custom *strobj = (lky_object_custom *)args->next->value;

    double val = OBJ_NUM_UNWRAP(numobj);
    char *str = lobjb_stringify((lky_object *)strobj);

    lky_object *ret = stlun_cinit(val, str);
    free(str);
    return ret;
}

lky_object *stlun_get_class()
{
    lky_object *me = lobj_alloc();
    return lobjb_build_func_ex(me, 2, (lky_function_ptr)stlun_build);
}
