#ifndef CONTEXT_H
#define CONTEXT_H

#include "ast.h"

void ctx_init();
void ctx_push_stack();
void ctx_pop_stack();

void ctx_set_var(char *frmt, ast_value_wrapper wrap);
ast_value_wrapper ctx_get_var(char *frmt);

void ctx_clean_up();

#endif