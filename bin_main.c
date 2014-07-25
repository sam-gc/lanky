#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lky_machine.h"
#include "lkyobj_builtin.h"

int main(int argc, char *argv[])
{
    if(argc == 1)
    {
        printf("Need some arguments!\n");
        return 0;
    }

    char print_assembly = 0;

    int i;
    for(i = 0; i < argc; i++)
    {
        if(!strcmp("-s", argv[i]))
            print_assembly = 1;
    }

    lky_object_code *code = lobjb_load_file(argv[1]);

    if(print_assembly)
        print_ops(code->ops, code->op_len);
    else
        mach_execute(code);

    return 0;
}