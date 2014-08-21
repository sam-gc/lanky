#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "parser.h"
#include "tools.h"
#include "ast_compiler.h"
#include "lky_machine.h"
#include "lky_object.h"
#include "lkyobj_builtin.h"
#include "lky_gc.h"
#include "stanky.h"
#include "stl_meta.h"

extern ast_node *programBlock;
extern int yyparse();
extern FILE *yyin;

int main(int argc, char *argv[])
{
    if(argc > 1)
    {
        yyin = fopen(argv[1], "r");
        if(!yyin)
        {
            printf("Error loading file %s\n", argv[1]);
            return 0;
        }

        // ctx_init();
           
        yyparse();
        lky_object_code *code = compile_ast(programBlock->next);
        write_to_file("test", code);
        ast_free(programBlock);

        int i;
        for(i = 0; i < argc; i++)
        {
            if(!strcmp(argv[i], "-s"))
            {
                print_ops(code->ops, code->op_len);
                return 0;
            }
        }

        // printf("%p ... %p ... %p\n", programBlock, programBlock->next, programBlock->next->next);
        printf("\nProgram output:\n==============================\n\n");
        
        arraylist list = arr_create(1);
        mach_interp interp = {NULL};

        gc_init();
        lky_object_function *func = (lky_object_function *)lobjb_build_func(code, 0, list, &interp);
        gc_add_root_object(func);

        func->bucket = lobj_alloc();
        func->bucket->members = get_stdlib_objects();
        trie_add(&func->bucket->members, "Meta", stlmeta_get_class(&interp));
        mach_execute((lky_object_function *)func);

        // eval(programBlock);
        // ast_node *n = ((ast_block_node *)programBlock)->payload;
        // ast_print(programBlock);

        // ast_binary_node *b = (ast_binary_node *)n;
        // printf("%c\n", b->opt);
        // ast_free(programBlock);
        // ctx_clean_up();
        printf("\n=============DEBUG============\n");
        // printf("Allocations: %d\tFrees: %d\n", get_malloc_count(), get_free_count());
        print_alloced();
        return 0;
    }
    else
    {
        gc_init();
        arraylist list = arr_create(1);
        mach_interp interp = {NULL};
        
        gc_init();
//        lky_object_function *func = (lky_object_function *)lobjb_build_func(NULL, 0, list, &interp);
//        gc_add_root_object(func);
//        
//        func->bucket = lobj_alloc();
//        func->bucket->members = get_stdlib_objects();
        
        stackframe frame;
        frame.bucket = lobj_alloc();
        frame.bucket->members = get_stdlib_objects();
        frame.parent_stack = list;
        
        interp.stack = &frame;
        
        gc_add_root_object(frame.bucket);
        
        trie_add(&frame.bucket->members, "Meta", stlmeta_get_class(&interp));
        
        run_repl(&interp);
    }
}
