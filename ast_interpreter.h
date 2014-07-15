#ifndef AST_INTERPRETER_H
#define AST_INTERPRETER_H

#include "ast.h"

ast_value_wrapper eval(ast_node *root);
void ast_print(ast_node *root);
void print_value(ast_value_wrapper wrap);
void value_wrapper_free(ast_value_wrapper wrap);

#endif