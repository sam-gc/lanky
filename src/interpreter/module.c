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

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <dlfcn.h>
#include "stl_string.h"
#include "lky_object.h"
#include "module.h"
#include "ast.h"
#include "parser.h"
#include "tools.h"
#include "ast_compiler.h"
#include "stl_meta.h"
#include "stanky.h"
#include "ast.h"
#include "lky_gc.h"
#include "hashtable.h"
#include "mempool.h"

#ifdef __APPLE__
#include <sys/syslimits.h>
#else
#include <linux/limits.h>
#endif

#define YY_BUF_SIZE 16384
#define EXISTS_READ(n) (access(n, R_OK) != -1)
extern ast_node *programBlock;
typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern YY_BUFFER_STATE yy_create_buffer(FILE *, size_t);
extern void yypush_buffer_state(YY_BUFFER_STATE);
extern void yypop_buffer_state();
extern char yyyhad_error;
static hashtable interpreters;

void md_wrap_dlclose(void *obj)
{
    dlclose(obj);
}

static lky_mempool mdsopool = {NULL, &md_wrap_dlclose};

long md_hash_interp(void *key, void *data)
{
    return (long)key;
}

int md_equ_interp(void *key, void *data)
{
    return key == data;
}

void md_free_subtable(void *key, void *val, void *data)
{
    hashtable *hst = (hashtable *)val;
    hst_free(hst);
    free(hst);
}

void md_mark_objects(void *key, void *val, void *data)
{
    gc_mark_object((lky_object *)val);
}

void md_mark_things_in_table(void *key, void *val, void *data)
{
    hashtable *hst = (hashtable *)val;
    hst_for_each(hst, md_mark_objects, NULL);
}

void md_init()
{
    interpreters = hst_create();
}

void md_unload()
{
    hst_for_each(&interpreters, md_free_subtable, NULL);
    hst_free(&interpreters);

    // Free the dll resources...
    pool_drain(&mdsopool);
}

void md_gc_cycle()
{
    hst_for_each(&interpreters, md_mark_things_in_table, NULL);
}

void md_get_full_filename(char *filename, char *buf)
{
    realpath(filename, buf);
}

void md_make_path(char *file, char *dir, char *extra, char *buf)
{
    strcpy(buf, dir);
    strcat(buf, extra);
    strcat(buf, "/");
    strcat(buf, file);
}

char *md_alloc_and_copy(char *text)
{
    char *nw = malloc(strlen(text) + 1);
    strcpy(nw, text);
    return nw;
}

char *md_lookup_text_code(char *file)
{
    char interm[1000];
    strcpy(interm, file);
    size_t len = strlen(file);
    if(len < 4 || !!strcmp(".lky", file + (len - 4)))
    {
        strcat(interm, ".lky");   
    }

    if(EXISTS_READ(interm))
        return md_alloc_and_copy(interm);

    return NULL;
}

char *md_lookup_lib(char *file, char *dir)
{   
    char interm[1000];
    strcpy(interm, file);
    size_t len = strlen(file);
    if(len < 3 || !!strcmp(".so", file + (len - 3)))
        strcat(interm, ".so");

    char tpath[2056];   
    md_make_path(interm, dir, "/modules_", tpath);

    if(EXISTS_READ(tpath)) 
        return md_alloc_and_copy(tpath);
    
    md_make_path(interm, "/usr/lib/lanky", "", tpath);

    if(EXISTS_READ(tpath))
        return md_alloc_and_copy(tpath);

    return NULL;
}

char *md_lookup_module(char *filename, char *codedir, int *lib)
{
    char sympath[strlen(filename) + strlen(codedir) + 1];
    strcpy(sympath, codedir);
    strcat(sympath, "/");
    strcat(sympath, filename);

    char fullname[PATH_MAX];
    md_get_full_filename(sympath, fullname);

    char *code = md_lookup_text_code(fullname);
    if(code)
    {
        *lib = 0;
        return code;
    }

    code = md_lookup_lib(filename, codedir);
    if(code)
    {
        *lib = 1;
        return code;
    }

    return NULL;       
}

hashtable *md_active_modules_for_interp(mach_interp *ip)
{
    hashtable *hst = hst_get(&interpreters, ip, md_hash_interp, md_equ_interp);
    if(!hst)
    {
        hashtable temp = hst_create();
        temp.duplicate_keys = 1;

        hst = malloc(sizeof(hashtable));
        memcpy(hst, &temp, sizeof(hashtable));
        hst_put(&interpreters, ip, hst, md_hash_interp, md_equ_interp);
    }

    return hst;
}

lky_object *md_load_text_code(char *fullname, mach_interp *ip)
{
    FILE *yyin = fopen(fullname, "r");
    if(!yyin)
        return NULL;

    YY_BUFFER_STATE buffer = yy_create_buffer(yyin, YY_BUF_SIZE);
    yypush_buffer_state(buffer);

    yyparse();

    gc_pause();
    lky_object_code *code = compile_ast_repl(programBlock->next);
    ast_free(programBlock);
    gc_resume();

    yypop_buffer_state();

    arraylist list = arr_create(1);

    lky_object_function *func = (lky_object_function *)lobjb_build_func(code, 0, list, ip);

    func->bucket = lobj_alloc();
    //func->bucket->members = get_stdlib_objects();
    hst_add_all_from(&func->bucket->members, &ip->stdlib, NULL, NULL);
    hst_put(&func->bucket->members, "Meta", stlmeta_get_class(ip), NULL, NULL);

    char pathtemp[strlen(fullname) + 1];
    strcpy(pathtemp, fullname);

    lobj_set_member(func->bucket, "dirname_", stlstr_cinit(dirname(pathtemp)));

    lky_object *ret = mach_execute((lky_object_function *)func);

    fclose(yyin);
    return ret;
}

lky_object *md_load_lib(char *fullname, char *file)
{
    char *shtemp;
    char shortname[1000];
    int i;
    for(i = strlen(fullname); i > 0; i--)
    {
        if(fullname[i - 1] != '/')
            continue;
        shtemp = fullname + i;
        break;
    }

    strcpy(shortname, shtemp);
    for(i = strlen(shtemp); i >= 0; i--)
        if(shortname[i] == '.')
            shortname[i] = '\0';

    char initname[strlen(shortname) + 6];
    sprintf(initname, "%s_init", shortname);

    void *lib_handle;
    lky_object *(*init_func)();

    lib_handle = dlopen(fullname, RTLD_LAZY);
    if(!lib_handle)
        printf("Couldn't load library.\n");

    init_func = dlsym(lib_handle, initname); 

    lky_object *obj = (*init_func)();
    pool_add(&mdsopool, lib_handle);

    return obj;
}

lky_object *md_load(char *filename, char *codedir, mach_interp *ip)
{   
    hashtable *hst = md_active_modules_for_interp(ip);

    int lib = 0;

    char *allocdname = md_lookup_module(filename, codedir, &lib);
    char fullname[PATH_MAX];
    strcpy(fullname, allocdname);
    free(allocdname);

    free(codedir);

    lky_object *ret = hst_get(hst, fullname, NULL, NULL);

    if(ret)
        return ret;

    if(lib) ret = md_load_lib(fullname, filename);
    else ret = md_load_text_code(fullname, ip);

    hst_put(hst, fullname, ret, NULL, NULL);

    return ret;
}
