#include "ast_unary_ops.h"
#include "ast_interpreter.h"
#include "ast_binary_ops.h"

void unary_print(ast_value_wrapper wrap)
{
    print_value(wrap);
}

ast_value_wrapper unary_not(ast_value_wrapper wrap)
{
    ast_value_wrapper ret;
    ret.type = VNONE;
    switch(wrap.type)
    {
    case VSTRING:
        ret.type = VINT;
        ret.value.i = 0;
        break;
    case VINT:
    case VDOUBLE:
        ret.type = VINT;
        ret.value.i = !NUMERIC_UNWRAP(wrap);
        break;
    case VNONE:
        ret.type = VINT;
        ret.value.i = 1;
        break;
    }

    return ret;
}

ast_value_wrapper unary_negative(ast_value_wrapper wrap)
{
    ast_value_wrapper ret;
    ret.type = VNONE;
    switch(wrap.type)
    {
    case VINT:
        ret.type = VINT;
        ret.value.i = -wrap.value.i;
        break;
    case VDOUBLE:
        ret.type = VDOUBLE;
        ret.value.d = -wrap.value.d;
        break;
    }

    return ret;
}