#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "stl_io.h"

lky_object *stlio_input(lky_object_seq *args, lky_object *func)
{
    lky_object *prompt = args->value;
    lobjb_print_object(prompt);

    char *buf;
    size_t sz = 0;
    getline(&buf, &sz, stdin);

    int len = strlen(buf);
    buf[len - 1] = '\0';
    
    lky_builtin_value v;
    v.s = buf;

    return lobjb_alloc(LBI_STRING, v);
}

lky_object *stlio_get_class()
{
    lky_object *obj = lobj_alloc();

    lobj_set_member(obj, "input", lobjb_build_func_ex(obj, 1, stlio_input));
    return obj;
}
