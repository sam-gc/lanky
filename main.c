#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "ast_interpreter.h"
#include "parser.h"
#include "tools.h"
#include "context.h"
#include "ast_compiler.h"
#include "lky_machine.h"
#include "lky_object.h"

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

    mach_execute(code);

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