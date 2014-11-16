#include <stdlib.h>
#include <stdio.h>
#include "module.h"
#include "ast.h"
#include "parser.h"
#include "tools.h"
#include "ast_compiler.h"
#include "stl_meta.h"
#include "stanky.h"
#include "ast.h"
#include "lky_gc.h"

#define YY_BUF_SIZE 16384
extern ast_node *programBlock;
typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern char yyyhad_error;
/*
        yyin = fopen(argv[1], "r");
        if(!yyin)
        {
            printf("Error loading file %s\n", argv[1]);
            return 0;
        }

        // ctx_init();
           
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

        // printf("%p ... %p ... %p\n", programBlock, programBlock->next, programBlock->next->next);
        // printf("\nProgram output:\n==============================\n\n");
        
//        gc_add_root_object(func);

        func->bucket = lobj_alloc();
        func->bucket->members = get_stdlib_objects();
        hst_put(&func->bucket->members, "Meta", stlmeta_get_class(&interp), NULL, NULL);
        mach_execute((lky_object_function *)func);*/


lky_object *md_load(char *filename, mach_interp *ip)
{   
    FILE *yyin = fopen(filename, "r");
    if(!yyin)
    {
        return NULL;
    }

    YY_BUFFER_STATE buffer = yy_create_buffer(yyin, YY_BUF_SIZE);
    yypush_buffer_state(buffer);

    yyparse();

    lky_object_code *code = compile_ast_repl(programBlock->next);

    yypop_buffer_state();

    arraylist list = arr_create(1);

    lky_object_function *func = (lky_object_function *)lobjb_build_func(code, 0, list, ip);

    func->bucket = lobj_alloc();
    func->bucket->members = get_stdlib_objects();
    hst_put(&func->bucket->members, "Meta", stlmeta_get_class(ip), NULL, NULL);

    gc_add_root_object(func);
    lky_object *ret = mach_execute((lky_object_function *)func);
    gc_remove_root_object(func);

    fclose(yyin);

    return ret;
}
