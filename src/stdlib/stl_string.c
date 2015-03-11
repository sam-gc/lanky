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
#include <string.h>
#include "stl_string.h"
#include "stl_array.h"
#include "hashtable.h"

lky_object *stlstr_stringify(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    return func->owner;
}

void stlstr_free(lky_object *obj)
{
    lky_object_custom *self = (lky_object_custom *)obj;
    
    free(self->data);
}

lky_object *stlstr_get_index(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    lky_object *obj = (lky_object *)args->value;
    long idx = OBJ_NUM_UNWRAP(obj);
    
    char *str = self->data;
    char s[2];
    sprintf(s, "%c", str[idx]);
    
    return stlstr_cinit(s);
}

lky_object *stlstr_hash(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    return lobjb_build_int(hst_djb2(self->data, NULL));
}

lky_object *stlstr_equals(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    lky_object_custom *obj = (lky_object_custom *)args->value;
    
    if((void *)obj->cls != (void *)stlstr_class())
        return &lky_nil;

    char *stra = self->data;
    char *strb = obj->data;
    
    return lobjb_build_int(!strcmp(stra, strb));
}

lky_object *stlstr_reverse(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    char *str = self->data;
    size_t len = strlen(str);

    char nstr[len + 1];
    int i = 0;
    for(i = 0; i < len; i++)
    {
        nstr[i] = str[len - i - 1];
    }

    nstr[i] = 0;

    return stlstr_cinit(nstr);
}

lky_object *stlstr_not_equals(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;

    lky_object_custom *obj = (lky_object_custom *)args->value;

    if((void *)obj->cls != (void *)stlstr_class())
        return &lky_nil;

    char *stra = self->data;
    char *strb = obj->data;

    return lobjb_build_int(!!strcmp(stra, strb));
}

lky_object *stlstr_greater_than(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;

    lky_object_custom *obj = (lky_object_custom *)args->value;

    if((void *)obj->cls != (void *)stlstr_class())
        return &lky_nil;

    char *stra = self->data;
    char *strb = obj->data;

    return lobjb_build_int(strcmp(stra, strb) > 0);
}

lky_object *stlstr_lesser_than(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;

    lky_object_custom *obj = (lky_object_custom *)args->value;

    if((void *)obj->cls != (void *)stlstr_class())
        return &lky_nil;

    char *stra = self->data;
    char *strb = obj->data;

    return lobjb_build_int(strcmp(stra, strb) < 0);
}

lky_object *stlstr_multiply(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    lky_object_builtin *other = (lky_object_builtin *)args->value;

    if(!((uintptr_t)(other) & 1) && other->type != LBI_INTEGER)
    {
        // TODO: Type error
        return &lky_nil;
    }

    char *str = self->data;
    long ct = OBJ_NUM_UNWRAP(other);

    // If the count is 0, we should just
    // return the empty string.
    if(!ct)
        return stlstr_cinit("");

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

    lky_object *ret = stlstr_cinit(nstr);
    free(nstr);

    return ret;
}

lky_object *stlstr_set_index(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    lky_object *idx = (lky_object *)args->value;
    lky_object_custom *nobj = (lky_object_custom *)args->next->value;
    
    long i = OBJ_NUM_UNWRAP(idx);
    
    char ch = ((char *)nobj->data)[0];
    
    char *str = self->data;
    
    str[i] = ch;
    
    return &lky_nil;
}

lky_object *stlstr_split(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    lky_object *other = (lky_object *)args->value;
    
    lky_object_function *strf = (lky_object_function *)lobj_get_member(other, "stringify_");
    if(!strf)
    {
        // TODO: Error
    }
    
    lky_func_bundle b = MAKE_BUNDLE(strf, NULL);
    lky_object *ostr = (lky_object *)(strf->callable.function)(&b);

    if((void *)ostr->cls != (void *)stlstr_class())
        return &lky_nil;

    char *delim = ((lky_object_custom *)ostr)->data;

    char *loc = self->data;
    size_t delen = strlen(delim);
    
    arraylist list = arr_create(10);

    if(delen == 0)
    {
        while(*loc)
        {
            char nc[2];
            nc[0] = *loc;
            nc[1] = '\0';
            arr_append(&list, stlstr_cinit(nc));
            loc++;
        }

        return stlarr_cinit(list);
    }
    
    while(loc)
    {
        char *next = strstr(loc, delim);
        size_t nlen = (next ? next : strlen(loc) + loc) - loc + 1;
        char *str = malloc(nlen);
        memcpy(str, loc, nlen - 1);
        str[nlen - 1] = '\0';
        
        arr_append(&list, stlstr_cinit(str));
        free(str);
        
        loc = next ? next + delen : NULL;
    }
    
    return stlarr_cinit(list);
}

lky_object *stlstr_fmt_func(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    char *mestr = self->data;

    arraylist list = arr_create(10);

    for(; args; args = args->next)
        arr_append(&list, args->value);

    lky_object *ret = stlstr_fmt_ext(mestr, list);

    arr_free(&list);
    return ret;
}

lky_object *stlstr_fmt_modulo(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    char *mestr = self->data;

    lky_object_custom *arg = (lky_object_custom *)args->value;
    arraylist list = stlarr_unwrap((lky_object *)arg);

    return stlstr_fmt_ext(mestr, list);
}

lky_object *stlstr_fmt_ext(char *mestr, arraylist list)
{
    char *buf = malloc(100);
    size_t buf_size = 100;
    size_t buf_len = 0;
    strcpy(buf, "");

    int aidx = 0;
    
    int i;
    for(i = 0; i < mestr[i]; i++)
    {
        if(mestr[i] != '@')
        {
            if(buf_len + 2 > buf_size)
            {
                buf_size *= 2;
                char *nb = malloc(buf_size);
                strcpy(nb, buf);
                free(buf);
                buf = nb;
            }

            char cc[2];
            sprintf(cc, "%c", mestr[i]);

            buf_len += 1;

            strcat(buf, cc);
            continue;
        }

        char *ostr = lobjb_stringify(arr_get(&list, aidx));
        size_t len = strlen(ostr);
        if(len + buf_len + 1 > buf_size)
        {
            buf_size *= 2;
            char *nb = malloc(buf_size);
            strcpy(nb, buf);
            free(buf);
            buf = nb;
        }

        aidx++;
        strcat(buf, ostr);
        free(ostr);
        buf_len += len;
    }

    lky_object *toret = stlstr_cinit(buf);
    free(buf);

    return toret;
}

lky_object *stlstr_iterable(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    char *mestr = self->data;
    int i;
    size_t len = strlen(mestr);
    arraylist list = arr_create(len + 1);
    
    for(i = 0; i < len; i++)
    {
        char c[2];
        c[0] = mestr[i];
        c[1] = '\0';

        arr_append(&list, stlstr_cinit(c));
    }

    return stlarr_cinit(list);
}

lky_object *stlstr_add(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    char *mestr = self->data;
    
    lky_object *other = (lky_object *)args->value;
    char sbf = OBJ_NUM_UNWRAP(((lky_object_builtin *)args->next->value));
    
    char *chr = lobjb_stringify(other);
    
    size_t len = strlen(chr) + strlen(mestr) + 1;
    char *newstr = malloc(len);
    
    char *first = sbf ? mestr : chr;
    char *second = first == mestr ? chr : mestr;
    
    strcpy(newstr, first);
    strcat(newstr, second);
    
    lky_object *ret = stlstr_cinit(newstr);
    free(newstr);
    free(chr);
    
    return ret;
}

char stlstr_escape_for(char i)
{
    switch(i)
    {
        case 'n':
            return '\n';
        case 't':
            return '\t';
        default:
            return '\\';
    }
}

char *stlstr_copy_and_escape(char *str)
{
    unsigned long len = strlen(str);
    unsigned long i, o;

    char *cop = calloc(len + 1, sizeof(char));
    for(i = o = 0; i < len; ++i, ++o)
    {
        if(str[i] != '\\')
        {
            cop[o] = str[i];
            continue;
        }

        i++;
        cop[o] = stlstr_escape_for(str[i]);
    }

    return cop;
}

lky_object *stlstr_to_lower(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    char *me = self->data;
    size_t len = strlen(me);
    char n[len + 1];
    int i;
    for(i = 0; i < len; i++)
    {
        char c = me[i];
        if(c >= 'A' && c <= 'Z')
            c = (c - 'A') + 'a';

        n[i] = c;
    }

    n[i] = '\0';

    return stlstr_cinit(n);
}

lky_object *stlstr_cinit(char *str)
{
    lky_object_custom *cobj = lobjb_build_custom(0);
    
    char *copied = stlstr_copy_and_escape(str);
    
    cobj->data = copied;
    
    lky_object *obj = (lky_object *)cobj;
    
    lobj_set_class(obj, stlstr_class());
    lobj_set_member(obj, "length", lobjb_build_int(strlen(copied)));
    lobj_set_member(obj, "reverse", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlstr_reverse));
    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlstr_stringify));
    lobj_set_member(obj, "split", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_split));
    lobj_set_member(obj, "fmt", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlstr_fmt_func));
    lobj_set_member(obj, "toLower", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlstr_to_lower));
    lobj_set_member(obj, "op_get_index_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_get_index));
    lobj_set_member(obj, "op_set_index_", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlstr_set_index));
    lobj_set_member(obj, "op_equals_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_equals));
    lobj_set_member(obj, "op_add_", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlstr_add));
    lobj_set_member(obj, "op_notequal_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_not_equals));
    lobj_set_member(obj, "op_gt_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_greater_than));
    lobj_set_member(obj, "op_lt_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_lesser_than));
    lobj_set_member(obj, "op_multiply_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_multiply));
    lobj_set_member(obj, "op_modulo_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_fmt_modulo));
    lobj_set_member(obj, "hash_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlstr_hash));
    lobj_set_member(obj, "iterable_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlstr_iterable));
    
    cobj->freefunc = stlstr_free;
    
    return (lky_object *)obj;
}

static lky_object *_stlstr_class = NULL;
lky_object *stlstr_class()
{
    if(!_stlstr_class)
        _stlstr_class = lobjb_build_int((long)&stlstr_cinit);
    return _stlstr_class;
}
