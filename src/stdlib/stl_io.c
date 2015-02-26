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

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <readline/readline.h>
#include "stl_io.h"
#include "stl_array.h"
#include "stl_string.h"
#include "arraylist.h"

typedef struct {
    FILE *f;
} stlio_file_object_data;

lky_object *stlio_input(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

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

lky_object *stlio_put(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *obj = (lky_object *)args->value;
    lobjb_print_object(obj);

    return &lky_nil;
}

lky_object *stlio_putln(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *obj = (lky_object *)args->value;
    lobjb_print(obj);

    return &lky_nil;
}

lky_object *stlio_printf(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    arraylist list = arr_create(10);
    MAKE_VA_ARGS(args, list, 1);
    lobjb_print_object(stlstr_fmt_ext(((lky_object_custom *)args->value)->data, list));

    return &lky_nil;
}

void stlio_file_dealloc(lky_object_custom *obj)
{
    free(obj->data);
}

lky_object *stlio_file_read_raw(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlio_file_object_data *data = self->data;

    FILE *f = data->f;

    char *buffer;
    long filelen;

    fseek(f, 0, SEEK_END);
    filelen = ftell(f);
    rewind(f);

    buffer = malloc((filelen + 1) * sizeof(char));
    fread(buffer, filelen, 1, f);

    lky_object_custom *str = (lky_object_custom *)stlstr_cinit("");
    free(str->data);
    str->data = buffer;

    lobj_set_member((lky_object *)self, "EOF", lobjb_build_int(1));
    lobj_set_member((lky_object *)str, "length", lobjb_build_int(filelen));

    return (lky_object *)str;
}

lky_object *stlio_file_readall(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlio_file_object_data *data = self->data;

    FILE *f = data->f;

    long ct = 100;
    char *running = calloc(ct, sizeof(char));
    char *line = NULL;
    size_t sz = 0;
    while(getline(&line, &sz, f) != -1)
    {
        size_t ls = strlen(line);
        if(strlen(running) + ls + 1 > ct)
        {
            ct *= 2;
            char *back = running;
            running = calloc(ct, sizeof(char));
            strcpy(running, back);
            free(back);
        }

        strcat(running, line);
        free(line);
        line = NULL;
        sz = 0;
    }

    lky_object *ret = stlstr_cinit(running);
    free(running);
    if(line)
        free(line);

    lobj_set_member((lky_object *)self, "EOF", lobjb_build_int(1));

    return ret;
}

lky_object *stlio_file_readlines(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

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

lky_object *stlio_file_readline(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

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

lky_object *stlio_file_write(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlio_file_object_data *data = self->data;

    FILE *f = data->f;

    lky_object *b = (lky_object *)args->value;
    char *line = lobjb_stringify(b);

    fprintf(f, "%s", line);

    free(line);

    return &lky_nil;
}

lky_object *stlio_file_writeline(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlio_file_object_data *data = self->data;

    FILE *f = data->f;

    lky_object *b = (lky_object *)args->value;
    char *line = lobjb_stringify(b);
    
    fprintf(f, "%s\n", line);

    free(line);

    return &lky_nil;
}

lky_object *stlio_file_close(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlio_file_object_data *data = self->data;

    fclose(data->f);
    data->f = NULL;
    return &lky_nil;
}

lky_object *stlio_make_file_object(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *obj = lobjb_build_custom(sizeof(stlio_file_object_data));
    lky_object *name = (lky_object *)args->value;
    lky_object *type = (lky_object *)args->next->value;

    if(name->type != LBI_STRING || type->type != LBI_STRING)
    {
        // TODO: Error
    }

    lky_object_custom *nb = (lky_object_custom *)name;
    lky_object_custom *nt = (lky_object_custom *)type;

    FILE *f = fopen(nb->data, nt->data);

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
    lobj_set_member(gobj, "getall", lobjb_build_func_ex(gobj, 0, (lky_function_ptr)stlio_file_readall));
    lobj_set_member(gobj, "readBytes", lobjb_build_func_ex(gobj, 0, (lky_function_ptr)stlio_file_read_raw));
    lobj_set_member(gobj, "EOF", lobjb_build_int(f ? 0 : 1));

    return gobj;
}

lky_object *stlio_get_class()
{
    lky_object *obj = lobj_alloc();

    lobj_set_member(obj, "prompt", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlio_input));
    lobj_set_member(obj, "put", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlio_put));
    lobj_set_member(obj, "putln", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlio_putln));
    lobj_set_member(obj, "printf", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlio_printf));
    lobj_set_member(obj, "fopen", lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlio_make_file_object));
    return obj;
}

lky_object *stl_io_init()
{
    return stlio_get_class();
}
