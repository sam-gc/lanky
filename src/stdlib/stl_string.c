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
    
    char *stra = self->data;
    char *strb = obj->data;
    
    return lobjb_build_int(!strcmp(stra, strb));
}

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
    lky_object_custom *self = func->owner;
    char *mestr = self->data;
    
    lky_object *other = args->value;
    char sbf = ((lky_object_builtin *)args->next->value)->value.i;
    
    lky_object *ostr = NULL;
    
    switch(other->type)
    {
        case LBI_CUSTOM:
        case LBI_CUSTOM_EX:
        {
            lky_object_function *strf = lobj_get_member(other, "stringify_");
            if(!strf)
            {
                // TODO: Error
            }
            
            ostr = (strf->callable.function)(NULL, strf);
//            ostr = ((lky_object_custom *)nstr)->data;
        }
        break;
        case LBI_FLOAT:
        case LBI_INTEGER:
        {
            ostr = lobjb_num_to_string(other);
        }
        break;
    }
    
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

lky_object *stlstr_cinit(char *str)
{
    lky_object_custom *cobj = lobjb_build_custom(0);
    
    char *copied = malloc(strlen(str) + 1);
    strcpy(copied, str);
    
    cobj->data = copied;
    
    lky_object *obj = (lky_object *)cobj;
    
    lobj_set_member(obj, "length", lobjb_build_int(strlen(copied)));
    lobj_set_member(obj, "op_add_", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlstr_add));
    lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlstr_stringify));
    lobj_set_member(obj, "split", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_split));
    lobj_set_member(obj, "op_get_index_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_get_index));
    lobj_set_member(obj, "op_set_index_", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlstr_set_index));
    lobj_set_member(obj, "op_equals_", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlstr_equals));
    
    cobj->freefunc = stlstr_free;
    
    return (lky_object *)obj;
}

//lky_object *stlstr_get_class()
//{
//    
//}