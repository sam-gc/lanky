#ifndef AST_MACHINE_H
#define AST_MACHINE_H

#include "lkyobj_builtin.h"

void mach_execute(lky_object_code *code);
void print_ops(char *ops, int tape_len);

#endif