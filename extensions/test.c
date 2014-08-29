#include "Lanky.h"

lky_object *testtest(lky_object_seq *args, lky_object *func)
{
    printf("Hello, world!\n");
    return &lky_nil;
}

lky_object *test_init()
{
    return lobjb_build_func_ex(NULL, 0, (lky_function_ptr)testtest);
}
