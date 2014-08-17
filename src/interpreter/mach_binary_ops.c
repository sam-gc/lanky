#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mach_binary_ops.h"
#include "lky_machine.h"

#define OBJ_NUM_PROMO(a, b) (a->type == LBI_FLOAT || b->type == LBI_FLOAT ? LBI_FLOAT : LBI_INTEGER) 
#define CHECK_EXEC_CUSTOM_IMPL(a, b, name) \
    do { \
        if(a->type == LBI_CUSTOM) { \
            lky_object *func = lobj_get_member(a, name); \
            if(!func || func->type != LBI_FUNCTION) \
                break; \
            lky_object_function *cfunc = (lky_object_function *)func;\
            if(cfunc->callable.argc != 1 || !cfunc->code) \
                break; \
            return bin_op_exec_custom(cfunc, b); \
        } \
    } while(0)

lky_object *bin_op_exec_custom(lky_object_function *func, lky_object *other)
{
    lky_object_code *code = func->code;

    lky_object *bucket = lobj_alloc();

    lobj_set_member(bucket, code->names[0], other);

    func->bucket = bucket;
    return mach_execute(func);
}

lky_object *lobjb_binary_add(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil || b == &lky_nil)
        return &lky_nil;

    if(ab->type == LBI_STRING || bb->type == LBI_STRING)
    {
        char *r, *l;
        char nr, nl;
        nr = nl = 0;
        if(ab->type != LBI_STRING)
        {
            l = malloc(100);
            str_print(ab->type, ab->value, l);
            r = bb->value.s;
            nl = 1;
        }
        else if(bb->type != LBI_STRING)
        {
            r = malloc(100);
            str_print(bb->type, bb->value, r);
            l = ab->value.s;
            nr = 1;
        }
        else
        {
            l = ab->value.s;
            r = bb->value.s;
        }

        char *n = malloc(strlen(r) + strlen(l) + 1);
        sprintf(n, "%s%s", l, r);
        lky_builtin_value v;
        v.s = n;

        if(nr)
            free(r);
        if(nl)
            free(l);

        return lobjb_alloc(LBI_STRING, v);
    }

    lky_builtin_value v;
    lky_builtin_type t = OBJ_NUM_PROMO(ab, bb);
    switch(t)
    {
        case LBI_FLOAT:
            v.d = OBJ_NUM_UNWRAP(ab) + OBJ_NUM_UNWRAP(bb);
            break;
        case LBI_INTEGER:
            v.i = OBJ_NUM_UNWRAP(ab) + OBJ_NUM_UNWRAP(bb);
            break;
        default:
            break;
    }

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_subtract(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil || b == &lky_nil)
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = OBJ_NUM_PROMO(ab, bb);
    switch(t)
    {
        case LBI_FLOAT:
            v.d = OBJ_NUM_UNWRAP(ab) - OBJ_NUM_UNWRAP(bb);
            break;
        case LBI_INTEGER:
            v.i = OBJ_NUM_UNWRAP(ab) - OBJ_NUM_UNWRAP(bb);
            break;
        default:
            break;
    }

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_multiply(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    lky_builtin_value v;
    lky_builtin_type t;

    if(a == &lky_nil || b == &lky_nil)
        return &lky_nil;

    if(ab->type == LBI_STRING || bb->type == LBI_STRING)
    {
        if(ab->type == LBI_STRING && bb->type == LBI_STRING)
            return &lky_nil;

        lky_object_builtin *strobj = ab->type == LBI_STRING ? ab : bb;
        lky_object_builtin *oobj = strobj == ab ? bb : ab;

        if(oobj->type != LBI_INTEGER)
            return &lky_nil;

        char *str = strobj->value.s;
        long ct = oobj->value.i;

        int len = strlen(str);
        int targ = len * ct;

        char *nstr = malloc(len * ct + 1);
        nstr[targ] = 0;
        strcpy(nstr, str);

        long done = len;
        while(done < targ)
        {
            long n = (done <= targ - done ? done : targ - done);
            memcpy(nstr + done, nstr, n);
            done += n;
        }

        v.s = nstr;
        t = LBI_STRING;

        return lobjb_alloc(t, v);
    }

    t = OBJ_NUM_PROMO(ab, bb);
    switch(t)
    {
        case LBI_FLOAT:
            v.d = OBJ_NUM_UNWRAP(ab) * OBJ_NUM_UNWRAP(bb);
            break;
        case LBI_INTEGER:
            v.i = OBJ_NUM_UNWRAP(ab) * OBJ_NUM_UNWRAP(bb);
            break;
        default:
            break;
    }

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_divide(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil || b == &lky_nil)
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = OBJ_NUM_PROMO(ab, bb);
    switch(t)
    {
        case LBI_FLOAT:
            v.d = OBJ_NUM_UNWRAP(ab) / OBJ_NUM_UNWRAP(bb);
            break;
        case LBI_INTEGER:
            v.i = OBJ_NUM_UNWRAP(ab) / OBJ_NUM_UNWRAP(bb);
            break;
        default:
            break;
    }

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_modulo(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil || b == &lky_nil)
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = OBJ_NUM_PROMO(ab, bb);
    switch(t)
    {
        case LBI_FLOAT:
            v.d = remainder(OBJ_NUM_UNWRAP(ab), OBJ_NUM_UNWRAP(bb));
            break;
        case LBI_INTEGER:
            v.i = ab->value.i % bb->value.i;
            break;
        default:
            break;
    }

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_lessthan(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil || b == &lky_nil)
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = LBI_INTEGER;

    if(ab->type == LBI_STRING || bb->type == LBI_STRING)
    {
        if(ab->type != LBI_STRING || bb->type != LBI_STRING)
            v.i = 0;
        else
            v.i = strcmp(ab->value.s, bb->value.s) < 0;
    }
    else
        v.i = (OBJ_NUM_UNWRAP(ab) < OBJ_NUM_UNWRAP(bb));

    // printf("==> %d\n", v.i);

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_greaterthan(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil || b == &lky_nil)
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = LBI_INTEGER;

    if(ab->type == LBI_STRING || bb->type == LBI_STRING)
    {
        if(ab->type != LBI_STRING || bb->type != LBI_STRING)
            v.i = 0;
        else
            v.i = strcmp(ab->value.s, bb->value.s) > 0;
    }
    else
        v.i = (OBJ_NUM_UNWRAP(ab) > OBJ_NUM_UNWRAP(bb));

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_equals(lky_object *a, lky_object *b)
{
    CHECK_EXEC_CUSTOM_IMPL(a, b, "op_equals_");

    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil || b == &lky_nil)
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = LBI_INTEGER;

    if(ab->type == LBI_STRING || bb->type == LBI_STRING)
    {
        if(ab->type != LBI_STRING || bb->type != LBI_STRING)
            v.i = 0;
        else
            v.i = strcmp(ab->value.s, bb->value.s) == 0;
    }
    else if(a->type == LBI_CUSTOM || b->type == LBI_CUSTOM)
        v.i = a == b;
    else
        v.i = (OBJ_NUM_UNWRAP(ab) == OBJ_NUM_UNWRAP(bb));

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_lessequal(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil || b == &lky_nil)
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = LBI_INTEGER;

    if(ab->type == LBI_STRING || bb->type == LBI_STRING)
    {
        if(ab->type != LBI_STRING || bb->type != LBI_STRING)
            v.i = 0;
        else
            v.i = strcmp(ab->value.s, bb->value.s) <= 0;
    }
    else
        v.i = (OBJ_NUM_UNWRAP(ab) <= OBJ_NUM_UNWRAP(bb));

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_greatequal(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil || b == &lky_nil)
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = LBI_INTEGER;

    if(ab->type == LBI_STRING || bb->type == LBI_STRING)
    {
        if(ab->type != LBI_STRING || bb->type != LBI_STRING)
            v.i = 0;
        else
            v.i = strcmp(ab->value.s, bb->value.s) >= 0;
    }
    else
        v.i = (OBJ_NUM_UNWRAP(ab) >= OBJ_NUM_UNWRAP(bb));

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_notequal(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil || b == &lky_nil)
        return &lky_nil;

    lky_builtin_value v;
    lky_builtin_type t = LBI_INTEGER;

    if(ab->type == LBI_STRING || bb->type == LBI_STRING)
    {
        if(ab->type != LBI_STRING || bb->type != LBI_STRING)
            v.i = 0;
        else
            v.i = strcmp(ab->value.s, bb->value.s) != 0;
    }
    else
        v.i = (OBJ_NUM_UNWRAP(ab) != OBJ_NUM_UNWRAP(bb));

    return lobjb_alloc(t, v);
}

lky_object *lobjb_binary_and(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil || b == &lky_nil)
        return lobjb_build_int(0);

    int vala = a->type == LBI_INTEGER || a->type == LBI_FLOAT ? OBJ_NUM_UNWRAP(ab) : 1;
    int valb = b->type == LBI_INTEGER || b->type == LBI_FLOAT ? OBJ_NUM_UNWRAP(bb) : 1;

    return lobjb_build_int(vala && valb);
}

lky_object *lobjb_binary_or(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(a == &lky_nil && b == &lky_nil)
        return lobjb_build_int(0);

    int vala = a->type == LBI_INTEGER || a->type == LBI_FLOAT ? OBJ_NUM_UNWRAP(ab) : 1;
    int valb = b->type == LBI_INTEGER || b->type == LBI_FLOAT ? OBJ_NUM_UNWRAP(bb) : 1;
    vala = a == &lky_nil ? 0 : vala;
    valb = b == &lky_nil ? 0 : valb;

    return lobjb_build_int(vala || valb);
}
