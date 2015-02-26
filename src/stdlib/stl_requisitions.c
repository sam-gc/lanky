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

#include "stl_requisitions.h"
#include "stl_string.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

void stlreq_wrap_dlclose(void *obj)
{
    dlclose(obj);
}

lky_mempool dlmempool = {NULL, &stlreq_wrap_dlclose};

lky_object *stlreq_import(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *nobj = (lky_object_custom *)args->value;
    char *pname = nobj->data;
    char *name = malloc(strlen(pname) + 6);
    char *shortname = malloc(strlen(pname) + 1);
    char *shrtloc = NULL;

    int i;
    for(i = strlen(pname); i >= 0; i--)
    {
        if(pname[i] != '/')
        {
            shortname[i] = pname[i];
            continue;
        }

        shrtloc = shortname + i + 1;
        break;
    }
    if(!shrtloc)
        shrtloc = shortname;

    char *initname = malloc(strlen(shrtloc) + 6);
    sprintf(initname, "%s_init", shrtloc);

    printf("%s\n", initname);

    free(shortname);

    sprintf(name, "./%s.so", pname);

    void *lib_handle;
    lky_object *(*init_func)();

    lib_handle = dlopen(name, RTLD_LAZY);
    if(!lib_handle)
    {
        printf("Couldn't load library.\n");
        // TODO: Error.
    }

    init_func = dlsym(lib_handle, initname);
    free(name);
    free(initname);

    char *error;
    if((error = dlerror()))
    {
        printf("Couldn't load init function.\n");
        // TODO: Error.
    }

    lky_object *obj = (*init_func)();
    pool_add(&dlmempool, lib_handle);

    return obj;
}

lky_object *stlreq_compile(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *fileobj = (lky_object_custom *)args->value;
    lky_object_custom *argobj = args->next ? (lky_object_custom *)args->next->value : NULL;

    printf("%s\n", argobj->data);

    char *filename = fileobj->data;

    unsigned long lastnum = strlen(filename) - 1;
    char *objname = calloc(lastnum + 2, sizeof(char));
    char *soname = calloc(lastnum + 3, sizeof(char));

    strcpy(objname, filename);
    strcpy(soname, filename);

    objname[lastnum] = 'o';
    soname[lastnum] = 's';
    soname[lastnum + 1] = 'o';

#ifdef __APPLE__
    char *compilefrmt = "gcc -shared -undefined dynamic_lookup -o %s %s %s";
#else
    char *compilefrmt = "gcc -shared -o %s %s %s -g -Isrc/interpreter -Isrc/compiler -Isrc/stdlib";
#endif

    char *compcmd = calloc(1000, sizeof(char));
    sprintf(compcmd, "gcc -fPIC -c %s -o %s -g -Isrc/interpreter -Isrc/compiler -Isrc/stdlib", filename, objname);
    system(compcmd);
    sprintf(compcmd, compilefrmt, soname, objname, argobj ? argobj->data : "");
    system(compcmd);

    free(soname);
    free(objname);
    
    return &lky_nil;
}

lky_object *stlreq_get_class()
{
    lky_object *obj = lobj_alloc();

#ifdef __APPLE__
    char *compcmd = "gcc -fPIC -c *.o\ngcc -shared -undefined dynamic_lookup -o <module name> *.o";
#else
    char *compcmd = "gcc -fPIC -c *.o\ngcc -shared -d -o <module name> *.o";
#endif
    
    lobj_set_member(obj, "import", lobjb_build_func_ex(obj, 1, (lky_function_ptr)&stlreq_import));
    lobj_set_member(obj, "compile", lobjb_build_func_ex(obj, 1, (lky_function_ptr)&stlreq_compile));
    lobj_set_member(obj, "buildCmd", stlstr_cinit(compcmd));
    
    return obj;
}
