#include "ast_binary_ops.h"
#include "tools.h"
#include "context.h"
#include "mempool.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void get_string_for_value(ast_value_wrapper val, char *buffer)
{
    if(val.type == VDOUBLE)
        sprintf(buffer, "%lf", val.value.d);
    else if(val.type == VINT)
        sprintf(buffer, "%lld", val.value.i);
}

ast_value_wrapper binary_add(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;

    if(left.type == VSTRING || right.type == VSTRING)
    {
        char *l;
        char *r;
        char *result;

        char il = 0;
        char ir = 0;

        if(left.type == VNONE)
        {
            l = "None";
            r = right.value.s;
        }
        else if(right.type == VNONE)
        {
            r = "None";
            l = left.value.s;
        }
        else if(IS_NUMERIC(left))
        {
            l = MALLOC(500);
            pool_add(&ast_memory_pool, l);
            get_string_for_value(left, l);
            il = 1;
            r = right.value.s;
        }
        else if(IS_NUMERIC(right))
        {
            r = MALLOC(500);
            pool_add(&ast_memory_pool, r);
            get_string_for_value(right, r);
            ir = 1;
            l = left.value.s;
        }
        else
        {
            r = right.value.s;
            l = left.value.s;
        }

        result = MALLOC(strlen(r) + strlen(l) + 1);
        pool_add(&ast_memory_pool, result);
        sprintf(result, "%s%s", l, r);

        // if(il)
        //     FREE(l);
        // if(ir)
        //     FREE(r);

        ret.value.s = result;
        ret.type = VSTRING; 
        return ret;
    }

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
    ret.type = VINT;

    if(left.type == VSTRING || right.type == VSTRING)
    {
        if(left.type != VSTRING || right.type != VSTRING)
        {
            ret.value.i = 0;
            return ret;
        }

        ret.value.i = !strcmp(left.value.s, right.value.s);
        return ret;
    }

    ret.value.i = NUMERIC_UNWRAP(left) == NUMERIC_UNWRAP(right);

    return ret;
}

ast_value_wrapper binary_greater(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;
    ret.type = VINT;

    if(left.type == VSTRING || right.type == VSTRING)
    {
        if(left.type != VSTRING || right.type != VSTRING)
        {
            ret.value.i = 0;
            return ret;
        }

        ret.value.i = strcmp(left.value.s, right.value.s) > 0;
        return ret;
    }

    ret.value.i = NUMERIC_UNWRAP(left) > NUMERIC_UNWRAP(right);

    return ret;
}

ast_value_wrapper binary_lesser(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;
    ret.type = VINT;

    if(left.type == VSTRING || right.type == VSTRING)
    {
        if(left.type != VSTRING || right.type != VSTRING)
        {
            ret.value.i = 0;
            return ret;
        }

        ret.value.i = strcmp(left.value.s, right.value.s) < 0;
        return ret;
    }

    ret.value.i = NUMERIC_UNWRAP(left) < NUMERIC_UNWRAP(right);

    return ret;
}

ast_value_wrapper binary_greaterequal(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;
    ret.type = VINT;

    if(left.type == VSTRING || right.type == VSTRING)
    {
        if(left.type != VSTRING || right.type != VSTRING)
        {
            ret.value.i = 0;
            return ret;
        }

        ret.value.i = strcmp(left.value.s, right.value.s) >= 0;
        return ret;
    }

    ret.value.i = NUMERIC_UNWRAP(left) >= NUMERIC_UNWRAP(right);

    return ret;
}

ast_value_wrapper binary_lesserequal(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;
    ret.type = VINT;

    if(left.type == VSTRING || right.type == VSTRING)
    {
        if(left.type != VSTRING || right.type != VSTRING)
        {
            ret.value.i = 0;
            return ret;
        }

        ret.value.i = strcmp(left.value.s, right.value.s) <= 0;
        return ret;
    }

    ret.value.i = NUMERIC_UNWRAP(left) <= NUMERIC_UNWRAP(right);

    return ret;
}

ast_value_wrapper binary_notequal(ast_value_wrapper left, ast_value_wrapper right)
{
    ast_value_wrapper ret;
    ret.type = VINT;

    if(left.type == VSTRING || right.type == VSTRING)
    {
        if(left.type != VSTRING || right.type != VSTRING)
        {
            ret.value.i = 0;
            return ret;
        }

        ret.value.i = !!strcmp(left.value.s, right.value.s);
        return ret;
    }

    ret.value.i = NUMERIC_UNWRAP(left) != NUMERIC_UNWRAP(right);

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

ast_value_wrapper binary_set_equal(ast_value_wrapper left, ast_value_wrapper right)
{
    if(left.type != VVAR)
    {
        DEBUG("Something went horribly wrong.");
        ast_value_wrapper wrap;
        wrap.type = VNONE;
        return wrap;
    }

    // Save strings...
    ast_value_wrapper tmp = right;
    if(right.type == VSTRING)
        tmp.value.s = alloc_str(right.value.s);

    ctx_set_var(left.value.s, tmp);
    return right;
}