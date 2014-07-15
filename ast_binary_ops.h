#ifndef AST_BINARY_OPS_H
#define AST_BINARY_OPS_H

#define NUMERIC_UNWRAP(wrap) (wrap.type == VINT ? wrap.value.i : wrap.value.d)
#define IS_NUMERIC(wrap) (wrap.type == VINT || wrap.type == VDOUBLE)

#include "ast.h"

ast_value_wrapper binary_add(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_subtract(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_multiply(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_divide(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_modulo(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_power(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_equal(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_greater(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_lesser(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_greaterequal(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_lesserequal(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_notequal(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_and(ast_value_wrapper left, ast_value_wrapper right);
ast_value_wrapper binary_or(ast_value_wrapper left, ast_value_wrapper right);

#endif