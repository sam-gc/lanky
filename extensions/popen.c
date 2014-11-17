#include "Lanky.h"

lky_object *popen_big_loop(lky_object_seq *args, lky_object *func)
{
    int i;
    for(i = 0; i < 1000000; i++)
    {
        printf("%d\n", i);
    }

    LKY_RETURN_NIL;
}

lky_object *popen_call(lky_object_seq *args, lky_object *func)
{
    lky_object_custom *c = LKY_CUST(LKY_FIRST_ARG(args));
    char *name = lobjb_stringify(c);
    printf("%s\n", name);
    FILE *fp;

    char buf[3000];

    fp = popen(name, "r");
    if(!fp)
    {
        printf("Failed to execute command %s\n", name);
        LKY_RETURN_NIL;
    }

    while(fgets(buf, sizeof(buf) - 1, fp))
    {
        printf("%s", buf);
    }

    pclose(fp);

    LKY_RETURN_NIL;
}

lky_object *popen_init()
{
    lky_object *obj = lobj_alloc();
    LKY_ADD_METHOD(obj, "bigLoop", 0, &popen_big_loop);
    LKY_ADD_METHOD(obj, "call", 1, &popen_call);

    return obj;
}
