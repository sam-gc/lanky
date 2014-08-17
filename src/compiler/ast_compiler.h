#ifndef AST_COMPILER_H
#define AST_COMPILER_H

#include "ast.h"
#include "lkyobj_builtin.h"

lky_object_code *compile_ast(ast_node *root);
void write_to_file(char *name, lky_object_code *code);

#endif