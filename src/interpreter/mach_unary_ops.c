#include "mach_unary_ops.h"

#define CHECK_EXEC_CUSTOM_IMPL(a, name) \
    do { \
        if(a->type == LBI_CUSTOM || a->type == LBI_CUSTOM_EX) {\
            lky_object *func = lobj_get_member(a, name); \
            if(!func || func->type != LBI_FUNCTION) \
                break;\
            lky_object_function *cfunc = (lky_object_function *)func;\
            if(cfunc->callable.argc && cfunc->callable.argc != 2) \
                break;\
            return un_op_exec_custom(cfunc); \
        } \
    } while(0)

lky_object *un_op_exec_custom(lky_object_function *func)
{
    lky_callable c = func->callable;

    return (lky_object *)c.function(NULL, (struct lky_object *)func);
}

lky_object *lobjb_unary_not(lky_object *a)
{
    CHECK_EXEC_CUSTOM_IMPL(a, "op_not_");   
    lky_object_builtin *ac = (lky_object_builtin *)a;
    switch(a->type)
    {
        case LBI_FLOAT:
        case LBI_INTEGER:
            return lobjb_build_int(!OBJ_NUM_UNWRAP(ac));
        default:
            break;
    }

    return lobjb_build_int(a == &lky_nil);
}
