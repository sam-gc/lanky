#include "context.h"
#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>

static Hashmap lanky_main_store;

void ctx_init()
{
    lanky_main_store = hm_create(100, 1);
}

void ctx_push_stack()
{

}

void ctx_pop_stack()
{

}

void ctx_set_var(char *frmt, ast_value_wrapper wrap)
{
    hm_put(&lanky_main_store, frmt, wrap);
}

ast_value_wrapper ctx_get_var(char *frmt)
{
    hm_error_t error;
    ast_value_wrapper val = hm_get(&lanky_main_store, frmt, &error);
    return val;
}

void ctx_clean_up()
{
    hm_free(&lanky_main_store);
}