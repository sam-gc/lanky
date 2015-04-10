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
#include <stdio.h>
#include <readline/readline.h>
#include "stl_io.h"
#include "stl_array.h"
#include "stl_string.h"
#include "arraylist.h"
#include "class_builder.h"

typedef struct {
    unsigned read: 1;
    unsigned write: 1;
    unsigned open: 1;
    FILE *f;
} stlio_blob;

CLASS_MAKE_METHOD(stlio_prompt, self,
    if($1) lobjb_print_object($1, interp_);

    char *buf = NULL;
    size_t sz = 0;
    getline(&buf, &sz, stdin);

    unsigned long len = strlen(buf);
    buf[len - 1] = '\0';
    
    lky_object *toret = stlstr_cinit(buf);
    free(buf);
    return toret;
)

CLASS_MAKE_METHOD(stlio_put, self,
    if($1) lobjb_print_object($1, interp_);
)

CLASS_MAKE_METHOD(stlio_putln, self,
    if($1) lobjb_print($1, interp_);
)

CLASS_MAKE_METHOD_EX(stlio_file_readall, self, stlio_blob *, fb_,
    CLASS_ERROR_ASSERT(fb_->read, "FileModeInvalid", "Attempted to read from write-only file.");
    CLASS_ERROR_ASSERT(fb_->open, "FileStreamClosed", "The file stream has already been closed");

    FILE *f = fb_->f;

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

        strcat(running, line); free(line); line = NULL;
        sz = 0;
    }

    lky_object *ret = stlstr_cinit(running);
    free(running);
    if(line)
        free(line);

    lobj_set_member(self, "EOF", &lky_yes);

    return ret;
)

CLASS_MAKE_METHOD_EX(stlio_file_readlines, self, stlio_blob *, fb_,
    CLASS_ERROR_ASSERT(fb_->read, "FileModeInvalid", "Attempted to read from write-only file.");
    CLASS_ERROR_ASSERT(fb_->open, "FileStreamClosed", "The file stream has already been closed");
    FILE *f = fb_->f;

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

    lobj_set_member(self, "EOF", &lky_yes);

    return stlarr_cinit(list);
)

CLASS_MAKE_METHOD_EX(stlio_file_readline, self, stlio_blob *, fb_,
    CLASS_ERROR_ASSERT(fb_->read, "FileModeInvalid", "Attempted to read from write-only file.");
    CLASS_ERROR_ASSERT(fb_->open, "FileStreamClosed", "The file stream has already been closed");
    FILE *f = fb_->f;

    char *line = NULL;
    size_t sz = 0;
    if(getline(&line, &sz, f) < 0)
    {
        lobj_set_member(self, "EOF", &lky_yes);
        if(line) free(line);
        return &lky_nil;
    }

    line[strlen(line) - 1] = '\0';

    lky_object *ret = stlstr_cinit(line);
    free(line);
    return ret;
)

CLASS_MAKE_METHOD_EX(stlio_file_write, self, stlio_blob *, fb_,
    CLASS_ERROR_ASSERT(fb_->write, "FileModeInvalid", "Attempting to write to file in read-only mode");
    CLASS_ERROR_ASSERT(fb_->open, "FileStreamClosed", "The file stream has already been closed");
    if(!$1)
        return &lky_nil;
    FILE *f = fb_->f;

    char *line = lobjb_stringify($1, interp_);

    fprintf(f, "%s", line);

    free(line);
)

CLASS_MAKE_METHOD_EX(stlio_file_writeline, self, stlio_blob *, fb_,
    CLASS_ERROR_ASSERT(fb_->write, "FileModeInvalid", "Attempting to write to file in read-only mode");
    CLASS_ERROR_ASSERT(fb_->open, "FileStreamClosed", "The file stream has already been closed");
    FILE *f = fb_->f;

    if(!$1)
    {
        fprintf(f, "\n");
        return &lky_nil;
    }

    char *line = lobjb_stringify($1, interp_);
    
    fprintf(f, "%s\n", line);

    free(line);
)

CLASS_MAKE_METHOD_EX(stlio_file_close, self, stlio_blob *, fb_,
    fclose(fb_->f);
    fb_->f = NULL;
    fb_->open = 0;
)

static lky_object *stlio_file_class_ = NULL;
lky_object *stlio_get_file_class()
{
    if(stlio_file_class_)
        return stlio_file_class_;

    CLASS_MAKE(cls, NULL, NULL, 0,
        CLASS_STATIC_ONLY;
        CLASS_PROTO("EOF", &lky_no);
        CLASS_PROTO_METHOD("close", stlio_file_close, 0);
        CLASS_PROTO_METHOD("getlns", stlio_file_readlines, 0);
        CLASS_PROTO_METHOD("getln", stlio_file_readline, 0);
        CLASS_PROTO_METHOD("getall", stlio_file_readall, 0);
        CLASS_PROTO_METHOD("put", stlio_file_write, 1);
        CLASS_PROTO_METHOD("putln", stlio_file_writeline, 1);
    );

    stlio_file_class_ = cls;
    return cls;
}

CLASS_MAKE_BLOB_FUNCTION(stlio_file_blob_function, stlio_blob *, b, how,
    if(how == CGC_FREE)
        free(b);
)

void stlio_file_custom_init(lky_object *self, lky_object *cls, void *data)
{
    CLASS_SET_BLOB(self, "fb_", data, stlio_file_blob_function);
}

CLASS_MAKE_METHOD(stlio_fopen, cls, 
    CLASS_ERROR_ASSERT($2, "MissingInformation", "Function requires filename and file mode.");
    char *mode = stlstr_unwrap($2);

    FILE *f = fopen(stlstr_unwrap($1), stlstr_unwrap($2));
    CLASS_ERROR_ASSERT(f, "FileNotFound", "The file was not found or was not accessible for the given mode.");

    stlio_blob *b = malloc(sizeof(stlio_blob));
    b->write = 0;
    b->read = 0;
    b->open = 1;

    b->f = f;
    size_t len = strlen(mode);
    int i;
    for(i = 0; i < len; i++)
    {
        if(mode[i] == '+')
            b->read = b->write = 1; // TODO: I know this is wrong...
        if(mode[i] == 'r' || mode[i] == 'R')
            b->read = 1;
        else if(mode[i] == 'w' || mode[i] == 'W' || mode[i] == 'a' || mode[i] == 'A')
            b->write = 1;
    }

    return clb_instantiate(stlio_get_file_class(), stlio_file_custom_init, b);
)

static lky_object *stlio_class_ = NULL;
lky_object *stlio_get_class()
{
    if(stlio_class_)
        return stlio_class_;

    CLASS_MAKE(cls, NULL, NULL, 0,       
        CLASS_STATIC_ONLY;
        CLASS_STATIC_METHOD("prompt", stlio_prompt, 1);
        CLASS_STATIC_METHOD("put", stlio_put, 1);
        CLASS_STATIC_METHOD("putln", stlio_putln, 1);
        CLASS_STATIC_METHOD("fopen", stlio_fopen, 2);
    );
    
    stlio_class_ = cls;
    return cls;
}

/*
lky_object *stlio_file_read_raw(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

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

    lky_object *str = stlstr_cinit("");
    free(stlstr_unwrap(str));
    CLASS_SET_BLOB(str, "sb_", buffer, stlstr_blob_func); 

    lobj_set_member((lky_object *)self, "EOF", lobjb_build_int(1));
    lobj_set_member((lky_object *)str, "length", lobjb_build_int(filelen));

    return (lky_object *)str;
}*/

