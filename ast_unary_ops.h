#ifndef AST_UNARY_OPS_H
#define AST_UNARY_OPS_H

#include "ast.h"
#include "ast_interpreter.h"

void unary_print(ast_value_wrapper wrap);
ast_value_wrapper unary_not(ast_value_wrapper wrap);
ast_value_wrapper unary_negative(ast_value_wrapper wrap);

#endif