#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include "ast.h"
#include "parser.h"
#include "tools.h"
#include "ast_compiler.h"
#include "lky_machine.h"
#include "lky_object.h"
#include "lkyobj_builtin.h"
#include "lky_gc.h"
#include "lky_object.h"
#include "stanky.h"
#include "tools.h"
#include "stl_meta.h"
#include "stl_string.h"
#include "stl_requisitions.h"
#include "stl_os.h"
#include "units.h"
#include "module.h"

extern ast_node *programBlock;
extern int yyparse();
extern FILE *yyin;

int main(int argc, char *argv[])
{
    un_setup();   
    md_init();
    stlos_init(argc - 1, argv + 1);
    if(argc > 1)
    {
        yyin = fopen(argv[1], "r");
        if(!yyin)
        {
            md_unload();
            un_clean();
            printf("Error loading file %s\n", argv[1]);
            return 0;
        }

        yyparse();
        lky_object_code *code = compile_ast_repl(programBlock->next);
        write_to_file("test", code);
        ast_free(programBlock);
        
        arraylist list = arr_create(1);
        mach_interp interp = {NULL};
        
        gc_init();
        lky_object_function *func = (lky_object_function *)lobjb_build_func(code, 0, list, &interp);

        int i;
        for(i = 0; i < argc; i++)
        {
            if(!strcmp(argv[i], "-S"))
            {
                stlmeta_examine(lobjb_make_seq_node((lky_object *)func), NULL);
                return 0;
            }
        }

        char codeloc[2000];
        realpath(argv[1], codeloc);

        char *path = dirname(codeloc);

        // printf("%p ... %p ... %p\n", programBlock, programBlock->next, programBlock->next->next);
        // printf("\nProgram output:\n==============================\n\n");
        
//        gc_add_root_object(func);
//
        interp.stdlib = get_stdlib_objects();

        func->bucket = lobj_alloc();
        hst_add_all_from(&func->bucket->members, &interp.stdlib, NULL, NULL);
        hst_put(&func->bucket->members, "Meta", stlmeta_get_class(&interp), NULL, NULL);
        lobj_set_member(func->bucket, "dirname_", stlstr_cinit(path));
        mach_execute((lky_object_function *)func);

        // eval(programBlock);
        // ast_node *n = ((ast_block_node *)programBlock)->payload;
        // ast_print(programBlock);

        // ast_binary_node *b = (ast_binary_node *)n;
        // printf("%c\n", b->opt);
        // ast_free(programBlock);
        // ctx_clean_up();
        // printf("\n=============DEBUG============\n");
        // printf("Allocations: %d\tFrees: %d\n", get_malloc_count(), get_free_count());
        // print_alloced();
        un_clean();
        md_unload();
        return 0;
    }
    else
    {
        gc_init();
        arraylist list = arr_create(1);

        mach_interp interp = {NULL, get_stdlib_objects()};
        
        gc_init();
//        lky_object_function *func = (lky_object_function *)lobjb_build_func(NULL, 0, list, &interp);
//        gc_add_root_object(func);
//        
//        func->bucket = lobj_alloc();
//        func->bucket->members = get_stdlib_objects();
        
        stackframe frame;
        frame.bucket = lobj_alloc();
        hst_add_all_from(&frame.bucket->members, &interp.stdlib, NULL, NULL);
        //frame.bucket->members = get_stdlib_objects();
        frame.parent_stack = list;
        frame.stack_size = 0;
        frame.locals_count = 0;
        
        interp.stack = &frame;
        
        gc_add_func_stack(&frame);
        
//        gc_add_root_object(frame.bucket);
        char path[1000];
        getcwd(path, 1000);

        hst_put(&frame.bucket->members, "Meta", stlmeta_get_class(&interp), NULL, NULL);
        lobj_set_member(frame.bucket, "dirname_", stlstr_cinit(path));
        
        run_repl(&interp);
    }

    pool_drain(&dlmempool);
    un_clean();
    md_unload();
}
