#ifndef MODULE_H
#define MODULE_H

#include "lkyobj_builtin.h"
#include "lky_machine.h"
#include "hashtable.h"

typedef struct {
    hashtable loaded;
} module;

lky_object *md_load(char *filename, mach_interp *ip);

#endif
