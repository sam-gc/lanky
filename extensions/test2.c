#include "Lanky.h"

lky_object *test2_print_message(lky_object_seq *args, lky_object *func)
{
    lky_object_custom *c = (lky_object_custom *)args->value;

    printf("Hello, %s\n", c->data);

    return lobjb_build_int(1);
}

lky_object *test2_init()
{
    return lobjb_build_func_ex(NULL, 1, (lky_function_ptr)test2_print_message);
}
