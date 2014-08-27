#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "stl_io.h"
#include "stl_array.h"
#include "stl_string.h"
#include "arraylist.h"

typedef struct {
    FILE *f;
} stlio_file_object_data;

lky_object *stlio_input(lky_object_seq *args, lky_object *func)
{
    lky_object *prompt = (lky_object *)args->value;
    lobjb_print_object(prompt);

    char *buf = NULL;
    size_t sz = 0;
    getline(&buf, &sz, stdin);

    unsigned long len = strlen(buf);
    buf[len - 1] = '\0';
    
    lky_object *toret = stlstr_cinit(buf);
    free(buf);
    return toret;
}

lky_object *stlio_put(lky_object_seq *args, lky_object *func)
{
    lky_object *obj = (lky_object *)args->value;
    lobjb_print_object(obj);

    return &lky_nil;
}

lky_object *stlio_putln(lky_object_seq *args, lky_object *func)
{
    lky_object *obj = (lky_object *)args->value;
    lobjb_print(obj);

    return &lky_nil;
}

void stlio_file_dealloc(lky_object_custom *obj)
{
    free(obj->data);
}

lky_object *stlio_file_readlines(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlio_file_object_data *data = self->data;
    
    FILE *f = data->f;

    arraylist list = arr_create(10);

    char *line = NULL;
    size_t sz = 0;
    while(getline(&line, &sz, f) != -1)
    {
        // Replace the newline character.
        line[strlen(line) - 1] = '\0';
        lky_object *str = stlstr_cinit(line);
        free(line);
        arr_append(&list, str);
        line = NULL;
        sz = 0;
    }
    
    if(line)
        free(line);

    lobj_set_member((lky_object *)self, "EOF", lobjb_build_int(1));

    return stlarr_cinit(list);
}

lky_object *stlio_file_readline(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlio_file_object_data *data = self->data;

    FILE *f = data->f;

    char *line = NULL;
    size_t sz = 0;
    if(getline(&line, &sz, f) < 0)
    {
        lobj_set_member((lky_object *)self, "EOF", lobjb_build_int(1));
        if(line) free(line);
        return &lky_nil;
    }

    line[strlen(line) - 1] = '\0';

    lky_object *ret = stlstr_cinit(line);
    free(line);
    return ret;
}

lky_object *stlio_file_write(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlio_file_object_data *data = self->data;

    FILE *f = data->f;

    lky_object_builtin *b = (lky_object_builtin *)args->value;
    char *line = b->value.s;

    fprintf(f, "%s", line);

    return &lky_nil;
}

lky_object *stlio_file_writeline(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlio_file_object_data *data = self->data;

    FILE *f = data->f;

    lky_object_builtin *b = (lky_object_builtin *)args->value;
    char *line = b->value.s;
    
    fprintf(f, "%s\n", line);

    return &lky_nil;
}

lky_object *stlio_file_close(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlio_file_object_data *data = self->data;

    fclose(data->f);
    data->f = NULL;
    return &lky_nil;
}

lky_object *stlio_make_file_object(lky_object_seq *args, lky_object *func)
{
    lky_object_custom *obj = lobjb_build_custom(sizeof(stlio_file_object_data));
    lky_object *name = (lky_object *)args->value;
    lky_object *type = (lky_object *)args->next->value;

    if(name->type != LBI_STRING || type->type != LBI_STRING)
    {
        // TODO: Error
    }

    lky_object_builtin *nb = (lky_object_builtin *)name;
    lky_object_builtin *nt = (lky_object_builtin *)type;

    FILE *f = fopen(nb->value.s, nt->value.s);
    
    stlio_file_object_data *data = malloc(sizeof(stlio_file_object_data));
    data->f = f;

    obj->data = data;
    obj->freefunc = (lobjb_custom_ex_dealloc_function)stlio_file_dealloc;
    
    lky_object *gobj = (lky_object *)obj;

    lobj_set_member(gobj, "close", lobjb_build_func_ex(gobj, 0, (lky_function_ptr)stlio_file_close));
    lobj_set_member(gobj, "getlns", lobjb_build_func_ex(gobj, 0, (lky_function_ptr)stlio_file_readlines));
    lobj_set_member(gobj, "getln", lobjb_build_func_ex(gobj, 0, (lky_function_ptr)stlio_file_readline));
    lobj_set_member(gobj, "put", lobjb_build_func_ex(gobj, 1, (lky_function_ptr)stlio_file_write));
    lobj_set_member(gobj, "putln", lobjb_build_func_ex(gobj, 1, (lky_function_ptr)stlio_file_writeline));
    lobj_set_member(gobj, "EOF", lobjb_build_int(0));

    return gobj;
}

lky_object *stlio_get_class()
{
    lky_object *obj = lobj_alloc();

    lobj_set_member(obj, "prompt", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlio_input));
    lobj_set_member(obj, "put", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlio_put));
    lobj_set_member(obj, "putln", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlio_putln));
    lobj_set_member(obj, "fopen", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlio_make_file_object));
    return obj;
}
