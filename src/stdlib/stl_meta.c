/* Lanky -- Scripting Language and Virtual Machine
 * Copyright (C) 2014  Sam Olsen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "stl_meta.h"
#include "stl_string.h"
#include "ast.h"
#include "parser.h"
#include "tools.h"
#include "ast_compiler.h"
#include "arraylist.h"
#include "lky_gc.h"
#include "instruction_set.h"
#include "colors.h"
#include "info.h"
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#define META_AUDIT(type) (printf(" -> %lu\t%s\n", sizeof(type), #type))

extern ast_node *programBlock;
typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern char yyyhad_error;

static int use_console_colors = 1;

void stlmeta_no_console_colors()
{
    use_console_colors = 0;
}

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
    lky_object_code *code = compile_ast_repl(programBlock->next);
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
            add_history(buf);
            continue;
        }
        
        lky_object *ret = compile_and_exec(line, interp);

        printf(YELLOW);
        char *str = lobjb_stringify(ret, interp);
        if(lobj_is_of_class(ret, stlstr_get_class()))
            printf("'%s'\n", str);
        else
            printf("%s\n", str);

        free(str);
        //lobjb_print(ret, interp);
        
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

lky_object *stlmeta_exec(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    lky_object_custom *strobj = (lky_object_custom *)args->value;
    char *str = strobj->data;
    
    return compile_and_exec(str, self->data);
}

lky_object *stlmeta_audit(lky_func_bundle *bundle)
{
    META_AUDIT(lky_object);
    META_AUDIT(lky_object_seq);
    META_AUDIT(lky_object_function);
    META_AUDIT(lky_object_custom);
    META_AUDIT(lky_object_code);
    META_AUDIT(lky_object_class);
    META_AUDIT(lky_object_error);
    META_AUDIT(lky_object_builtin);

    return &lky_nil;
}

lky_object *stlmeta_repl(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    
    run_repl(self->data);
    
    return &lky_nil;
}

lky_object *stlmeta_address_of(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    void *p = (void *)args->value;
    char str[20];
    sprintf(str, "%p", p);
    return stlstr_cinit(str);
}

lky_object *stlmeta_allow_int_tags(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *o = (lky_object *)args->value;
    lobjb_uses_pointer_tags_ = OBJ_NUM_UNWRAP(o);

    return &lky_nil;
}

lky_object *stlmeta_gc_pause(lky_func_bundle *bundle)
{
    gc_pause_collection();
    return &lky_nil;
}

lky_object *stlmeta_gc_halt(lky_func_bundle *bundle)
{
    gc_pause();
    return &lky_nil;
}

lky_object *stlmeta_gc_resume(lky_func_bundle *bundle)
{
    gc_resume();
    gc_resume_collection();
    return &lky_nil;
}

lky_object *stlmeta_gc_pass(lky_func_bundle *bundle)
{
    gc_gc();
    return &lky_nil;
}

lky_object *stlmeta_gc_collect(lky_func_bundle *bundle)
{
    gc_mark();
    gc_collect();
    return &lky_nil;
}

lky_object *stlmeta_gc_alloced(lky_func_bundle *bundle)
{
    return lobjb_build_int((long)gc_alloced());
}

int stlmeta_space_count_for_idx(int idx)
{
    if(idx < 10)
        return 5;
    if(idx < 100)
        return 4;
    if(idx < 1000)
        return 3;
    if(idx < 10000)
        return 2;
    if(idx < 100000)
        return 1;
    if(idx < 1000000)
        return 0;

    return 0;
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
        case LI_JUMP_TRUE_ELSE_POP:
            return "JUMP_TRUE_ELSE_POP";
        case LI_JUMP_FALSE_ELSE_POP:
            return "JUMP_FALSE_ELSE_POP";
        case LI_IGNORE:
            return "IGNORE";
        case LI_SAVE_LOCAL:
            return "SAVE_LOCAL";
        case LI_LOAD_LOCAL:
            return "LOAD_LOCAL";
        case LI_PUSH_NIL:
            return "PUSH_NIL";
        case LI_PUSH_NEW_OBJECT:
            return "PUSH_NEW_OBJECT";
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
        case LI_MAKE_TABLE:
            return "MAKE_TABLE";
        case LI_MAKE_OBJECT:
            return "MAKE_OBJECT";
        case LI_LOAD_INDEX:
            return "LOAD_INDEX";
        case LI_SAVE_INDEX:
            return "SAVE_INDEX";
        case LI_DDUPLICATE:
            return "DDUPLICATE";
        case LI_SINK_FIRST:
            return "SINK_FIRST";
        case LI_MAKE_ITER:
            return "MAKE_ITER";
        case LI_NEXT_ITER_OR_JUMP:
            return "NEXT_ITER_OR_JUMP";
        case LI_UNARY_NOT:
            return "UNARY_NOT";
        case LI_UNARY_NEGATIVE:
            return "UNARY_NEGATIVE";
        case LI_PUSH_CATCH:
            return "PUSH_CATCH";
        case LI_POP_CATCH:
            return "POP_CATCH";
        default:
            return "";
    }
}

void stlmeta_print_dissassembly(lky_object_code *code)
{
    long i, j;
    for(i = 0; i < code->op_len; i++)
    {
        int numspaces = stlmeta_space_count_for_idx(i);
        for(j = 0; j < numspaces; j++) printf(" ");

        lky_instruction instr = code->ops[i];
        printf("%ld: %s", i, stlmeta_string_for_instruction(instr));
        
        switch(instr)
        {
            case LI_LOAD_CONST:
            {
                unsigned int idx = *(unsigned int *)(code->ops + (++i));
                printf("\t%d\t(", idx);
                i += 3;
                lobjb_print_object(code->constants[code->ops[i]], NULL);
                printf(")");
                break;
            }
            case LI_LOAD_CLOSE:
            case LI_SAVE_CLOSE:
            case LI_LOAD_MEMBER:
            case LI_SAVE_MEMBER:
            {
                unsigned int idx = *(unsigned int *)(code->ops + (++i));
                printf("\t%d\t", idx);
                i += 3;
                //printf("\t%d\t(\"%s\")", code->ops[++i], code->names[code->ops[i]]);
                break;
            }
            case LI_CALL_FUNC:
                printf("\t%d\t[argc]", code->ops[++i]);
                break;
            case LI_JUMP:
            case LI_JUMP_FALSE:
            case LI_JUMP_TRUE:
            case LI_JUMP_TRUE_ELSE_POP:
            case LI_JUMP_FALSE_ELSE_POP:
            case LI_NEXT_ITER_OR_JUMP:
            case LI_PUSH_CATCH:
            {
                unsigned int idx = *(unsigned int *)(code->ops + (++i));
                printf("\t%u\t[jump location]", idx);
                i += 3;
                
                break;
            }
            case LI_LOAD_LOCAL:
            case LI_SAVE_LOCAL:
            {
                unsigned int idx = *(unsigned int *)(code->ops + (++i));
                printf("\t%d\t[local index]", idx);
                i += 3;
                break;
            }
            case LI_MAKE_ARRAY:
            {
                unsigned int idx = *(unsigned int *)(code->ops + (++i));
                printf("\t%u\t[item count]", idx);
                i += 3;
                
                break;
            }
            case LI_MAKE_TABLE:
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
            case LI_MAKE_OBJECT:
            {
                unsigned int idx = *(unsigned int *)(code->ops + (++i));
                printf("\t%u\t[item count]", idx);
                i += 3 + idx;

                break;
            }
            default: break;
        }
        
        printf("\n");
    }
}

lky_object *stlmeta_examine(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_function *obj = (lky_object_function *)args->value;

    if((uintptr_t)(obj) & 1)
    {
        printf("Lanky tagged pointer.\n");
        return &lky_nil;
    }

    if(obj->type != LBI_FUNCTION)
    {
        printf("Lanky object.\n");

        return &lky_nil;
    }

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
        lobjb_print_object(code->constants[i], BUW_INTERP(bundle));
        
        if(i < code->num_constants - 1)
            printf(", ");
    }
    
    printf("]\n\n");

    printf("Number of local slots: %ld\n", code->num_locals);
    stlmeta_print_dissassembly(code);
    
    return &lky_nil;
}

lky_object *stlmeta_help_stdlib(lky_func_bundle *bundle)
{
    printf("LANKY STANDARD LIBRARY\n"
           "======================\n\n"
           "+ Object (stl_object.c)\n"
           "\t- stringify_()\n"
           "\t- op_equals_()\n"
           "\t- members_()\n\n"
           "+ Array (stl_array.c)\n"
           "\t- reverse()\n"
           "\t- contains(obj)\n"
           "\t- removeAt(index)\n"
           "\t- count\n"
           "\t- indexOf(obj)\n"
           "\t- last()\n"
           "\t- insert(obj, index)\n"
           "\t- append(obj)\n"
           "\t- forEach(callback)\n"
           "\t- joined(str)\n\n"
           "+ Convert (stl_convert.c)\n"
           "\t- toInt(obj)\n"
           "\t- Unit(value, str)\n"
           "\t- units(unit, str)\n"
           "\t- toFloat(obj)\n"
           "\t- toString(obj)\n\n"
           "+ Io (stl_io.c)\n"
           "\t- prompt(string)\n"
           "\t- putln(string)\n"
           "\t- put(string)\n"
           "\t- printf(format, args)\n"
           "\t- fopen(filename, mode)\n\n"
           "+ Math (stl_math.c)\n"
           "\t- All standard C -lm functions\n"
           "\t- Phys\n"
           "\t- Astro\n"
           "\t- randInt(upper, lower)\n"
           "\t- rand()\n"
           "\t- pi\n"
           "\t- e\n\n"
           "+ Meta (stl_meta.c)\n"
           "\t- repl()\n"
           "\t- exec(string)\n"
           "\t- examine(obj)\n"
           "\t- helpStdlib()\n\n"
           "+ OS (stl_os.c)\n"
           "\t- argc\n"
           "\t- argv\n\n"
           "+ C (stl_requisitions.c)\n"
           "\t- import(filename) \tReplaced by 'load'\n"
           "\t- buildCmd\n"
           "\t- compile(filename, flags)\n\n"
           "+ String (stl_string.c)\n"
           "\t- reverse()\n"
           "\t- length\n"
           "\t- fmt(args)\n"
           "\t- split(delim)\n\n"
           "+ Hash Table (stl_hashtable.c)\n"
           "\t- keys()\n"
           "\t- addAll(other)\n"
           "\t- hasKey(key)\n"
           "\t- count\n"
           "\t- removeValue(val)\n"
           "\t- remove(key)\n"
           "\t- values()\n"
           "\t- hasValue(val)\n"
           "\t- size_\n\n"
           "+ Time (stl_time.c)\n"
           "\t- unix()\n\n"
           ".\n"
           ".\n"
           ".\n");

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
    lobj_set_member(obj, "helpStdlib", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlmeta_help_stdlib));
    lobj_set_member(obj, "gc_pause", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlmeta_gc_pause));
    lobj_set_member(obj, "gc_resume", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlmeta_gc_resume));
    lobj_set_member(obj, "gc_pass", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlmeta_gc_pass));
    lobj_set_member(obj, "gc_collect", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlmeta_gc_collect));
    lobj_set_member(obj, "gc_alloced", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlmeta_gc_alloced));
    lobj_set_member(obj, "gc_halt", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlmeta_gc_halt));
    lobj_set_member(obj, "addressOf", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlmeta_address_of));
    lobj_set_member(obj, "allowIntTags", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlmeta_allow_int_tags));
    lobj_set_member(obj, "audit", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlmeta_audit));
    lobj_set_member(obj, "version", stlstr_cinit(LKY_VERSION_NUM));
    lobj_set_member(obj, "versionTag", stlstr_cinit(LKY_VERSION_TAG));
    lobj_set_member(obj, "copyright", stlstr_cinit(LKY_COPYRIGHT));
    
    return obj;
}
