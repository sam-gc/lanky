#include <stdlib.h>
#include <string.h>
#include "stl_string.h"
#include "stl_array.h"

lky_object *stlstr_stringify(lky_object_seq *args, lky_object_function *func)
{
    return func->owner;
}

void stlstr_free(lky_object *obj)
{
    lky_object_custom *self = (lky_object_custom *)obj;
    
    free(self->data);
}

lky_object *stlstr_get_index(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    lky_object *obj = (lky_object *)args->value;
    long idx = OBJ_NUM_UNWRAP(obj);
    
    char *str = self->data;
    char s[2];
    sprintf(s, "%c", str[idx]);
    
    return stlstr_cinit(s);
}

lky_object *stlstr_equals(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    lky_object_custom *obj = (lky_object_custom *)args->value;
    
    if(obj->cls != (struct lky_object *)stlstr_class())
        return &lky_nil;

    char *stra = self->data;
    char *strb = obj->data;
    
    return lobjb_build_int(!strcmp(stra, strb));
}

lky_object *stlstr_not_equals(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;

    lky_object_custom *obj = (lky_object_custom *)args->value;

    if(obj->cls != (struct lky_object *)stlstr_class())
        return &lky_nil;

    char *stra = self->data;
    char *strb = obj->data;

    return lobjb_build_int(!!strcmp(stra, strb));
}

lky_object *stlstr_greater_than(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;

    lky_object_custom *obj = (lky_object_custom *)args->value;

    if(obj->cls != (struct lky_object *)stlstr_class())
        return &lky_nil;

    char *stra = self->data;
    char *strb = obj->data;

    return lobjb_build_int(strcmp(stra, strb) > 0);
}

lky_object *stlstr_lesser_than(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;

    lky_object_custom *obj = (lky_object_custom *)args->value;

    if(obj->cls != (struct lky_object *)stlstr_class())
        return &lky_nil;

    char *stra = self->data;
    char *strb = obj->data;

    return lobjb_build_int(strcmp(stra, strb) < 0);
}

lky_object *stlstr_multiply(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    lky_object_builtin *other = (lky_object_builtin *)args->value;

    if(other->type != LBI_INTEGER)
    {
        // TODO: Type error
        return &lky_nil;
    }

    char *str = self->data;
    long ct = OBJ_NUM_UNWRAP(other);

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

//    if(ab->type == LBI_STRING || bb->type == LBI_STRING)
//    {
//        if(ab->type == LBI_STRING && bb->type == LBI_STRING)
//            return &lky_nil;
//
//        lky_object_builtin *strobj = ab->type == LBI_STRING ? ab : bb;
//        lky_object_builtin *oobj = strobj == ab ? bb : ab;
//
//        if(oobj->type != LBI_INTEGER)
//            return &lky_nil;
//
//        char *str = strobj->value.s;
//        long ct = oobj->value.i;
//
//        int len = strlen(str);
//        int targ = len * ct;
//
//        char *nstr = malloc(len * ct + 1);
//        nstr[targ] = 0;
//        strcpy(nstr, str);
//
//        long done = len;
//        while(done < targ)
//        {
//            long n = (done <= targ - done ? done : targ - done);
//            memcpy(nstr + done, nstr, n);
//            done += n;
//        }
//
//        v.s = nstr;
//        t = LBI_STRING;
//

lky_object *stlstr_set_index(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    lky_object *idx = (lky_object *)args->value;
    lky_object_custom *nobj = (lky_object_custom *)args->next->value;
    
    long i = OBJ_NUM_UNWRAP(idx);
    
    char ch = ((char *)nobj->data)[0];
    
    char *str = self->data;
    
    str[i] = ch;
    
    return &lky_nil;
}

lky_object *stlstr_split(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    lky_object *other = (lky_object *)args->value;
    
    lky_object_function *strf = (lky_object_function *)lobj_get_member(other, "stringify_");
    if(!strf)
    {
        // TODO: Error
    }
    
    lky_object *ostr = (lky_object *)(strf->callable.function)(NULL, (struct lky_object *)strf);

    if(ostr->cls != stlstr_class())
        return &lky_nil;

    char *delim = ((lky_object_custom *)ostr)->data;
    
    char *loc = self->data;
    size_t delen = strlen(delim);
    
    arraylist list = arr_create(10);
    
//    // We want to capture the first instance.
//    char *next = strstr(loc, delim);
//    size_t nlen = (next ? next : strlen(loc) + loc) - loc + 1;
//    char *str = malloc(nlen);
//    memcpy(str, )
    
    while(loc)
    {
        char *next = strstr(loc, delim);
        size_t nlen = (next ? next : strlen(loc) + loc) - loc + 1;
        char *str = malloc(nlen);
        memcpy(str, loc, nlen - 1);
        str[nlen - 1] = '\0';
        
//        loc = next;
        
        arr_append(&list, stlstr_cinit(str));
        free(str);
        
        loc = next ? next + delen : NULL;
    }
    
    return stlarr_cinit(list);
}

lky_object *stlstr_add(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    char *mestr = self->data;
    
    lky_object *other = (lky_object *)args->value;
    char sbf = ((lky_object_builtin *)args->next->value)->value.i;
    
    lky_object *ostr = NULL;
    
    switch(other->type)
    {
        case LBI_CUSTOM:
        case LBI_CUSTOM_EX:
        {
            lky_object_function *strf = (lky_object_function *)lobj_get_member(other, "stringify_");
            if(!strf)
            {
                // TODO: Error
            }
            
            ostr = (lky_object *)(strf->callable.function)(NULL, (struct lky_object *)strf);
//            ostr = ((lky_object_custom *)nstr)->data;
        }
        break;
        case LBI_FLOAT:
        case LBI_INTEGER:
        {
            ostr = lobjb_num_to_string(other);
        }
        break;
        default:
            break;
    }
    
    if(ostr->cls != stlstr_class())
        return &lky_nil;

    char *chr = ((lky_object_custom *)ostr)->data;
    
    size_t len = strlen(chr) + strlen(mestr) + 1;
    char *newstr = malloc(len);
    
    char *first = sbf ? mestr : chr;
    char *second = first == mestr ? chr : mestr;
    
    strcpy(newstr, first);
    strcat(newstr, second);
    
    lky_object *ret = stlstr_cinit(newstr);
    free(newstr);
    
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

lky_object *stlstr_cinit(char *str)
{
    lky_object_custom *cobj = lobjb_build_custom(0);
    
    char *copied = stlstr_copy_and_escape(str);
    
    cobj->data = copied;
    
    lky_object *obj = (lky_object *)cobj;
    
    lobj_set_class(obj, stlstr_class());
    lobj_set_member(obj, "length", lobjb_build_int(strlen(copied)));
    lobj_set_member(obj, "op_add_", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlstr_add));
    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlstr_stringify));
    lobj_set_member(obj, "split", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_split));
    lobj_set_member(obj, "op_get_index_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_get_index));
    lobj_set_member(obj, "op_set_index_", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlstr_set_index));
    lobj_set_member(obj, "op_equals_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_equals));
    lobj_set_member(obj, "op_notequal_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_not_equals));
    lobj_set_member(obj, "op_gt_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_greater_than));
    lobj_set_member(obj, "op_lt_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_lesser_than));
    lobj_set_member(obj, "op_multiply_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_multiply));
    
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

//lky_object *stlstr_get_class()
//{
//    
//}
