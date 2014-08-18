#ifndef AST_MACHINE_H
#define AST_MACHINE_H

#include "lkyobj_builtin.h"

lky_object *mach_execute(lky_object_function *func);
void print_ops(char *ops, int tape_len);

#endif