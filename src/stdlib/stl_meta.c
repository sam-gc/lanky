#include "stl_meta.h"
#include "ast.h"
#include "parser.h"
#include "tools.h"
#include "ast_compiler.h"
#include "arraylist.h"
#include "lky_gc.h"
#include "instruction_set.h"
#include "colors.h"
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

extern ast_node *programBlock;
typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern char yyyhad_error;

int needs_multiline(char *line)
{
    int b = 0;
    
    int len = (int)strlen(line);
    int i;
    for(i = 0; i < len; i++)
    {
        char c = line[i];
        if(c == '(' || c == '{' || c == '[')
            b++;
        if(c == ')' || c == '}' || c == ']')
            b--;
    }
    
    if(!b)
        return b;
    
    return b < 0 ? -1 : 1;
}

void print_indents(int b)
{
    int i;
    for(i = 0; i < b; i++)
    {
        printf("\001" DARK_GREY "\002" "....");
    }
}

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
    char *prompt = "\001" LIGHT_BLUE "\002" "> " "\001" DEFAULT "\002";

    char *buf;
    
//    rl_bind_key('\t', rl_abort); // We don't want autocomplete.
    char *line = malloc(1);
    strcpy(line, "");
    
    int bmulti = 0;

    while((buf = readline(prompt)) != NULL)
    {
        if (strcmp(buf,"quit") == 0)
            break;
        
        char *tmp = line;
        line = malloc(strlen(tmp) + strlen(buf) + 1);
        strcpy(line, tmp);
        strcat(line, buf);
        
        free(tmp);
        
        if(needs_multiline(line))
        {
            bmulti += needs_multiline(buf);
            print_indents(bmulti);
            prompt = "    \001" DEFAULT "\002";
            continue;
        }
        
//        printf(WHITE);
        lky_object *ret = compile_and_exec(line, interp);

        printf(DEFAULT);
        lobjb_print(ret);
        
        if (buf[0] != 0)
            add_history(buf);
        
        free(buf);
        free(line);
        line = malloc(1);
        strcpy(line, "");
        prompt = "\001" LIGHT_BLUE "\002" "> " "\001" DEFAULT "\002";
        bmulti = 0;
    }
    
    free(line);
    free(buf);
}

lky_object *stlmeta_exec(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    lky_object_custom *strobj = (lky_object_custom *)args->value;
    char *str = strobj->data;
    
    return compile_and_exec(str, self->data);
}

lky_object *stlmeta_repl(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    run_repl(self->data);
    
    return &lky_nil;
}

char *stlmeta_string_for_instruction(lky_instruction instr)
{
    // Switch statement generated using "instrgen.lky".
    switch(instr)
    {
        case LI_BINARY_ADD:
            return "BINARY_ADD";
        case LI_BINARY_SUBTRACT:
            return "BINARY_SUBTRACT";
        case LI_BINARY_MULTIPLY:
            return "BINARY_MULTIPLY";
        case LI_BINARY_DIVIDE:
            return "BINARY_DIVIDE";
        case LI_BINARY_MODULO:
            return "BINARY_MODULO";
        case LI_BINARY_LT:
            return "BINARY_LT";
        case LI_BINARY_GT:
            return "BINARY_GT";
        case LI_BINARY_EQUAL:
            return "BINARY_EQUAL";
        case LI_BINARY_LTE:
            return "BINARY_LTE";
        case LI_BINARY_GTE:
            return "BINARY_GTE";
        case LI_BINARY_NE:
            return "BINARY_NE";
        case LI_BINARY_AND:
            return "BINARY_AND";
        case LI_BINARY_OR:
            return "BINARY_OR";
        case LI_LOAD_CONST:
            return "LOAD_CONST";
        case LI_PRINT:
            return "PRINT";
        case LI_POP:
            return "POP";
        case LI_JUMP_FALSE:
            return "JUMP_FALSE";
        case LI_JUMP_TRUE:
            return "JUMP_TRUE";
        case LI_JUMP:
            return "JUMP";
        case LI_IGNORE:
            return "IGNORE";
        case LI_SAVE_LOCAL:
            return "SAVE_LOCAL";
        case LI_LOAD_LOCAL:
            return "LOAD_LOCAL";
        case LI_PUSH_NIL:
            return "PUSH_NIL";
        case LI_CALL_FUNC:
            return "CALL_FUNC";
        case LI_RETURN:
            return "RETURN";
        case LI_LOAD_MEMBER:
            return "LOAD_MEMBER";
        case LI_SAVE_MEMBER:
            return "SAVE_MEMBER";
        case LI_MAKE_FUNCTION:
            return "MAKE_FUNCTION";
        case LI_MAKE_CLASS:
            return "MAKE_CLASS";
        case LI_SAVE_CLOSE:
            return "SAVE_CLOSE";
        case LI_LOAD_CLOSE:
            return "LOAD_CLOSE";
        case LI_MAKE_ARRAY:
            return "MAKE_ARRAY";
        case LI_LOAD_INDEX:
            return "LOAD_INDEX";
        case LI_SAVE_INDEX:
            return "SAVE_INDEX";
        default:
            return "";
    }
}

void stlmeta_print_dissassembly(lky_object_code *code)
{
    long i;
    for(i = 0; i < code->op_len; i++)
    {
        lky_instruction instr = code->ops[i];
        printf("%ld: %s", i, stlmeta_string_for_instruction(instr));
        
        switch(instr)
        {
            case LI_BINARY_ADD:
            case LI_BINARY_SUBTRACT:
            case LI_BINARY_MULTIPLY:
            case LI_BINARY_DIVIDE:
            case LI_BINARY_MODULO:
            case LI_BINARY_LT:
            case LI_BINARY_GT:
            case LI_BINARY_EQUAL:
            case LI_BINARY_LTE:
            case LI_BINARY_GTE:
            case LI_BINARY_NE:
            case LI_BINARY_AND:
            case LI_BINARY_OR:
            case LI_PRINT:
            case LI_POP:
            case LI_IGNORE:
            case LI_PUSH_NIL:
            case LI_CALL_FUNC:
            case LI_RETURN:
            case LI_LOAD_INDEX:
            case LI_SAVE_INDEX:
                break;
            case LI_LOAD_CONST:
                printf("\t%d\t(", code->ops[++i]);
                lobjb_print_object(code->constants[code->ops[i]]);
                printf(")");
                break;
            case LI_LOAD_CLOSE:
            case LI_SAVE_CLOSE:
            case LI_LOAD_MEMBER:
            case LI_SAVE_MEMBER:
                printf("\t%d\t(\"%s\")", code->ops[++i], code->names[code->ops[i]]);
                break;
            case LI_JUMP:
            case LI_JUMP_FALSE:
            case LI_JUMP_TRUE:
            {
                unsigned int idx = *(unsigned int *)(code->ops + (++i));
                printf("\t%u\t[jump location]", idx);
                i += 3;
                
                break;
            }
            case LI_LOAD_LOCAL:
            case LI_SAVE_LOCAL:
                printf("\tUnimplemented");
                break;
            case LI_MAKE_ARRAY:
            {
                unsigned int idx = *(unsigned int *)(code->ops + (++i));
                printf("\t%u\t[item count]", idx);
                i += 3;
                
                break;
            }
            case LI_MAKE_CLASS:
            case LI_MAKE_FUNCTION:
                printf("\t%d\t[argc]", code->ops[++i]);
                break;
        }
        
        printf("\n");
    }
}

lky_object *stlmeta_examine(lky_object_seq *args, lky_object_function *func)
{
    lky_object_function *obj = (lky_object_function *)args->value;
    lky_object_code *code = obj->code;
    
    if(!code)
    {
        printf("C extension function at %p taking %d argument%s.\n", obj->callable.function, obj->callable.argc, obj->callable.argc == 1 ? "" : "s");
        return &lky_nil;
    }
    
    printf("Lanky native function taking %d argument%s.\n", obj->callable.argc, obj->callable.argc == 1 ? "" : "s");
    
    int i;
    printf("Names: \n[");
    
    for(i = 0; i < code->num_names; i++)
    {
        printf("\"%s\"%s", code->names[i], i == code->num_names - 1 ? "" : ", ");
    }
    
    printf("]\n\nConstants: \n[");
    
    for(i = 0; i < code->num_constants; i++)
    {
        lobjb_print_object(code->constants[i]);
        
        if(i < code->num_constants - 1)
            printf(", ");
    }
    
    printf("]\n\n");
    stlmeta_print_dissassembly(code);
    
    return &lky_nil;
}

lky_object *stlmeta_get_class(mach_interp *interp)
{
    lky_object_custom *custom = lobjb_build_custom(sizeof(mach_interp));
    custom->freefunc = NULL;
    custom->savefunc = NULL;
    custom->data = interp;
    
    lky_object *obj = (lky_object *)custom;

    lobj_set_member(obj, "exec", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlmeta_exec));
    lobj_set_member(obj, "repl", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlmeta_repl));
    lobj_set_member(obj, "examine", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlmeta_examine));
    
    return obj;
}