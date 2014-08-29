#include "stl_requisitions.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

lky_mempool dlmempool = {NULL, &dlclose};

lky_object *stlreq_import(lky_object_seq *args, lky_object_function *func)
{
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
    }
    if(!shrtloc)
        shrtloc = shortname;

    char *initname = malloc(strlen(shrtloc) + 6);
    sprintf(initname, "%s_init", shrtloc);

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

lky_object *stlreq_get_class()
{
    return lobjb_build_func_ex(NULL, 1, (lky_function_ptr)&stlreq_import);            
}
