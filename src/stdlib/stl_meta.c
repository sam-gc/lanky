#include "stl_meta.h"
#include "ast.h"
#include "parser.h"
#include "tools.h"
#include "ast_compiler.h"
#include "arraylist.h"
#include "lky_gc.h"
#include "instruction_set.h"
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

extern ast_node *programBlock;
typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern char yyyhad_error;

lky_object *compile_and_exec(char *str, mach_interp *interp)
{
    // We want to handle errors properly.
    yyyhad_error = 0;
    
    // Talk to flex/bison
    YY_BUFFER_STATE buffer = yy_scan_string(str);
    yyparse();
    yy_delete_buffer(buffer);
    
    if(yyyhad_error)
    {
        printf("    --> Did you forget your semicolon?\n");
        return &lky_nil;
    }
    
    gc_pause();
    lky_object_code *code = compile_ast(programBlock->next);
    ast_free(programBlock);
    gc_resume();
    
    // We want to remove the last pop so that we
    // can get the return value of the last statement.
    long i;
    for(i = code->op_len - 1; i >= 0; i--)
    {
        if(code->ops[i] == LI_POP || code->ops[i] == LI_IGNORE)
        {
            code->ops[i] = LI_IGNORE;
            break;
        }
    }
    
    arraylist t;
    lky_object_function *tor = (lky_object_function *)lobjb_build_func(code, 0, t, interp);
    
    lky_object *ret = mach_interrupt_exec(tor);
    
    return ret;
}

void run_repl(mach_interp *interp)
{
    char *buf;
    
//    rl_bind_key('\t', rl_abort); // We don't want autocomplete.
    
    while((buf = readline("\n==$ ")) != NULL)
    {
        if (strcmp(buf,"quit") == 0)
            break;
        
        lky_object *ret = compile_and_exec(buf, interp);
        lobjb_print(ret);
        
        if (buf[0] != 0)
            add_history(buf);
        
        free(buf);
    }
    
    free(buf);
}

lky_object *stlmeta_exec(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    lky_object_builtin *strobj = args->value;
    char *str = strobj->value.s;
    
    return compile_and_exec(str, self->data);
}

lky_object *stlmeta_get_class(mach_interp *interp)
{
    lky_object_custom *custom = lobjb_build_custom(sizeof(mach_interp));
    custom->freefunc = NULL;
    custom->savefunc = NULL;
    custom->data = interp;

    lobj_set_member(custom, "exec", lobjb_build_func_ex(custom, 1, (lky_function_ptr)stlmeta_exec));
    
    return custom;
}