#include <stdio.h>
#include "ast.h"
#include "ast_interpreter.h"
#include "parser.h"
#include "tools.h"

extern ast_node *programBlock;
extern int yyparse();

int main(int argc, char *argv[])
{
    yyparse();
    // printf("%p ... %p ... %p\n", programBlock, programBlock->next, programBlock->next->next);
    ast_value_wrapper val = eval(programBlock);
    // printf("%d %lf\n", val.type, val.value.d);
    print_value(val);
    // ast_node *n = ((ast_block_node *)programBlock)->payload;
    // ast_print(programBlock);

    // ast_binary_node *b = (ast_binary_node *)n;
    // printf("%c\n", b->opt);
    ast_free(programBlock);
    printf("Allocations: %d\tFrees: %d\n", get_malloc_count(), get_free_count());
    return 0;
}