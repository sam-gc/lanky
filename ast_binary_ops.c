#include "ast_binary_ops.h"
#include <math.h>

ast_value_wrapper binary_add(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    // TODO: make this compatible with objects...
    if(left.type == VDOUBLE || right.type == VDOUBLE)
    {
        ret.value.d = NUMERIC_UNWRAP(left) + NUMERIC_UNWRAP(right);
        ret.type = VDOUBLE;
    }
    else
    {
        ret.value.i = NUMERIC_UNWRAP(left) + NUMERIC_UNWRAP(right);
        ret.type = VINT;
    }

    return ret;
}

ast_value_wrapper binary_subtract(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    // TODO: make this compatible with objects...
    if(left.type == VDOUBLE || right.type == VDOUBLE)
    {
        ret.value.d = NUMERIC_UNWRAP(left) - NUMERIC_UNWRAP(right);
        ret.type = VDOUBLE;
    }
    else
    {
        ret.value.i = NUMERIC_UNWRAP(left) - NUMERIC_UNWRAP(right);
        ret.type = VINT;
    }

    return ret;
}

ast_value_wrapper binary_multiply(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    // TODO: make this compatible with objects...
    if(left.type == VDOUBLE || right.type == VDOUBLE)
    {
        ret.value.d = NUMERIC_UNWRAP(left) * NUMERIC_UNWRAP(right);
        ret.type = VDOUBLE;
    }
    else
    {
        ret.value.i = NUMERIC_UNWRAP(left) * NUMERIC_UNWRAP(right);
        ret.type = VINT;
    }

    return ret;
}

ast_value_wrapper binary_divide(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    // TODO: make this compatible with objects...
    if(left.type == VDOUBLE || right.type == VDOUBLE)
    {
        ret.value.d = NUMERIC_UNWRAP(left) / NUMERIC_UNWRAP(right);
        ret.type = VDOUBLE;
    }
    else
    {
        ret.value.i = NUMERIC_UNWRAP(left) / NUMERIC_UNWRAP(right);
        ret.type = VINT;
    }

    return ret;
}

ast_value_wrapper binary_modulo(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    // TODO: make this compatible with objects...
    ret.value.i = (long long)NUMERIC_UNWRAP(left) % (long long)NUMERIC_UNWRAP(right);
    ret.type = VINT;

    return ret;
}

ast_value_wrapper binary_power(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    ret.value.d = pow(NUMERIC_UNWRAP(left), NUMERIC_UNWRAP(right));
    ret.type = VDOUBLE;

    return ret;
}

ast_value_wrapper binary_equal(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    ret.value.i = NUMERIC_UNWRAP(left) == NUMERIC_UNWRAP(right);
    ret.type = VINT;

    return ret;
}

ast_value_wrapper binary_greater(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    ret.value.i = NUMERIC_UNWRAP(left) > NUMERIC_UNWRAP(right);
    ret.type = VINT;

    return ret;
}

ast_value_wrapper binary_lesser(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    ret.value.i = NUMERIC_UNWRAP(left) < NUMERIC_UNWRAP(right);
    ret.type = VINT;

    return ret;
}

ast_value_wrapper binary_greaterequal(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    ret.value.i = NUMERIC_UNWRAP(left) >= NUMERIC_UNWRAP(right);
    ret.type = VINT;

    return ret;
}

ast_value_wrapper binary_lesserequal(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    ret.value.i = NUMERIC_UNWRAP(left) <= NUMERIC_UNWRAP(right);
    ret.type = VINT;

    return ret;
}

ast_value_wrapper binary_notequal(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    ret.value.i = NUMERIC_UNWRAP(left) != NUMERIC_UNWRAP(right);
    ret.type = VINT;

    return ret;
}

ast_value_wrapper binary_and(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    ret.value.i = NUMERIC_UNWRAP(left) && NUMERIC_UNWRAP(right);
    ret.type = VINT;

    return ret;
}

ast_value_wrapper binary_or(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    ret.value.i = NUMERIC_UNWRAP(left) || NUMERIC_UNWRAP(right);
    ret.type = VINT;

    return ret;
}