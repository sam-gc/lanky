#include "lkyobj_builtin.h"
#include <stdlib.h>
#include <string.h>

#define OBJ_NUM_PROMO(a, b) (a->type == LBI_FLOAT || b->type == LBI_FLOAT ? LBI_FLOAT : LBI_INTEGER) 
#define BI_CAST(o, n) lky_object_builtin * n = (lky_object_builtin *) o

lky_object *lobjb_alloc(lky_builtin_type t, lky_builtin_value v)
{
    lky_object_builtin *obj = malloc(sizeof(lky_object_builtin));
    obj->type = t;
    obj->mem_count = 0;
    obj->members = arr_create(10);
    obj->value = v;

    return (lky_object *)obj;
}

lky_object *lobjb_build_int(long value)
{
    lky_builtin_value v;
    v.i = value;
    return lobjb_alloc(LBI_INTEGER, v);
}

lky_object *lobjb_build_float(double value)
{
    lky_builtin_value v;
    v.d = value;
    return lobjb_alloc(LBI_FLOAT, v);
}

void str_print(lky_builtin_type t, lky_builtin_value v, char *buf)
{
    switch(t)
    {
        case LBI_FLOAT:
            sprintf(buf, "%lf", v.d);
        break;
        case LBI_INTEGER:
            sprintf(buf, "%ld", v.i);
        break;
    }
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

        char *nstr = malloc(strlen(str) * ct + 1);
        strcpy(nstr, "");

        long i;
        for(i = 0; i < ct; i++)
            strcat(nstr, str);

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

    v.i = (OBJ_NUM_UNWRAP(ab) < OBJ_NUM_UNWRAP(bb));

    // printf("==> %d\n", v.i);

    return lobjb_alloc(t, v);
}

char lobjb_quick_compare(lky_object *a, lky_object *b)
{
    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if(ab->type == LBI_STRING || bb->type == LBI_STRING)
    {
        if(ab->type != LBI_STRING || bb->type != LBI_STRING)
            return 0;

        return !strcmp(ab->value.s, bb->value.s);
    }

    if(a == &lky_nil || b == &lky_nil)
    {
        return a == b;
    }

    return OBJ_NUM_UNWRAP(ab) == OBJ_NUM_UNWRAP(bb);
}

void lobjb_print(lky_object *a)
{
    lky_object_builtin *b = (lky_object_builtin *)a;

    switch(b->type)
    {
        case LBI_FLOAT:
            printf("%lf\n", b->value.d);
        break;
        case LBI_INTEGER:
            printf("%ld\n", b->value.i);
        break;
        case LBI_STRING:
            printf("%s\n", b->value.s);
        break;
        case LBI_NIL:
            printf("(nil)\n");
        break;
    }
}

void lobjb_serialize(lky_object *o, FILE *f)
{
    BI_CAST(o, obj);

    fwrite(&(obj->type), sizeof(int), 1, f);
    switch(obj->type)
    {
        case LBI_FLOAT:
        {
            unsigned long sz = sizeof(double);
            fwrite(&sz, sizeof(unsigned long), 1, f);
            fwrite(&(obj->value.d), sizeof(double), 1, f);
        }
        break;
        case LBI_INTEGER:
        {
            unsigned long sz = sizeof(long);
            fwrite(&sz, sizeof(unsigned long), 1, f);
            fwrite(&(obj->value.d), sizeof(long), 1, f);
        }
        break;
        case LBI_STRING:
        {
            char *str = obj->value.s;
            unsigned long sz = strlen(str) + 1;
            fwrite(&sz, sizeof(unsigned long), 1, f);
            fwrite(str, sizeof(char), sz, f);
        }
        break;
    }
}

lky_object *lobjb_deserialize(FILE *f)
{
    lky_builtin_type type;
    fread(&type, sizeof(int), 1, f);

    unsigned long sz;
    fread(&sz, sizeof(unsigned long), 1, f);

    lky_builtin_value value;

    switch(type)
    {
        case LBI_FLOAT:
            fread(&value.d, sz, 1, f);
        break;
        case LBI_INTEGER:
            fread(&value.i, sz, 1, f);
        break;
        case LBI_STRING:
        {
            char *str = malloc(sz);
            fread(str, sz, 1, f);
            value.s = str;
        }
        break;
    }

    return lobjb_alloc(type, value);
}

lky_object_code *lobjb_load_file(char *name)
{
    FILE *f = fopen(name, "r");
    long len;
    fread(&len, sizeof(long), 1, f);

    arraylist con = arr_create(len + 1);

    long i;
    for(i = 0; i < len; i++)
    {
        lky_object *obj = lobjb_deserialize(f);
        rc_incr(obj);
        arr_append(&con, obj);
    }

    fread(&len, sizeof(long), 1, f);
    char *ops = malloc(len);
    fread(ops, sizeof(char), len, f);

    lky_object_code *obj = malloc(sizeof(lky_object_code));
    obj->constants = con;
    obj->ops = ops;
    obj->op_len = len;

    return obj;
}

void lobjb_clean(lky_object *a)
{
    lky_object_builtin *obj = (lky_object_builtin *)a;

    switch(obj->type)
    {
        case LBI_STRING:
            free(obj->value.s);
        break;
    }
}
