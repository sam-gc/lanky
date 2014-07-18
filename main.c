#include <stdio.h>
#include "ast.h"
#include "ast_interpreter.h"
#include "parser.h"
#include "tools.h"
#include "context.h"

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

    ctx_init();
    yyparse();
    // printf("%p ... %p ... %p\n", programBlock, programBlock->next, programBlock->next->next);
    printf("\nProgram output:\n==============================\n\n");
    eval(programBlock);
    // ast_node *n = ((ast_block_node *)programBlock)->payload;
    // ast_print(programBlock);

    // ast_binary_node *b = (ast_binary_node *)n;
    // printf("%c\n", b->opt);
    ast_free(programBlock);
    ctx_clean_up();
    printf("\n=============DEBUG============\n");
    printf("Allocations: %d\tFrees: %d\n", get_malloc_count(), get_free_count());
    return 0;
}