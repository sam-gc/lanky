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
#include "aquarium.h"
#include "lky_object.h"
#include "stanky.h"
#include "tools.h"
#include "stl_meta.h"
#include "stl_string.h"
#include "stl_requisitions.h"
#include "stl_os.h"
#include "stl_array.h"
#include "units.h"
#include "module.h"
#include "serialize.h"
#include "colors.h"
#include "info.h"
#include "exporter.h"

#define STRINGIFY_TOK(tok) #tok
#define STRINGIFY_INT(i) STRINGIFY_TOK(i)

// TODO-----
// HEY YOU,
//      This is a transitional period. The function calling mechanism is moving
//      to a more robust model (we can now control what things are getting sent
//      to functions by using the lky_func_bundle structure). As such you might
//      need to replace old api calls. The following is a search/replace string
//      for vim that will do what you want.
// %s/\(lky_object_seq\|lky_object\) \*args, \(lky_object_function\|lky_object\) \*func)\n{/lky_func_bundle *bundle)\r{\r    lky_object_function *func = BUW_FUNC(bundle);\r    lky_object_seq *args = BUW_ARGS(bundle);\r/gc)

extern ast_node *programBlock;
extern int yyparse();
extern FILE *yyin;

hashtable parse_args(int argc, char *argv[])
{
    hashtable tab = hst_create();
    hst_put(&tab, "-o", "a.out", NULL, NULL);

    int i;
    for(i = 0; i < argc; i++)
    {
        if(strcmp(argv[i], "-o") == 0 && i < argc - 1)
            hst_put(&tab, "-o", argv[++i], NULL, NULL);
        else if(strcmp(argv[i], "-c") == 0)
            hst_put(&tab, "-c", (void *)1, NULL, NULL);
        else if(strcmp(argv[i], "-S") == 0)
            hst_put(&tab, "-S", (void *)1, NULL, NULL);
        else if(strcmp(argv[i], "-e") == 0)
            hst_put(&tab, "-e", (void *)1, NULL, NULL);
        else if(strcmp(argv[i], "--no-tagged-ints") == 0)
            hst_put(&tab, "--no-tagged-ints", (void *)1, NULL, NULL);
        else if(strcmp(argv[i], "--use-system-malloc") == 0)
            hst_put(&tab, "--use-system-malloc", (void *)1, NULL, NULL);
        else if(strcmp(argv[i], "-b") == 0) 
        {
            hst_put(&tab, "-b", (void *)1, NULL, NULL);
            hst_put(&tab, "-o", "lky_bottled.c", NULL, NULL);
        }
    }

    return tab;
}

void exec_in_repl()
{
    char *start;
#ifdef __clang__
    start = "Lanky (\"" LIGHT_BLUE LKY_VERSION_TAG DEFAULT "\"), version " LKY_VERSION_NUM ";\ncompiled " __DATE__ " for "
#ifdef __APPLE__
       "Mac OS X"
#else
       "GNU/Linux"
#endif 
       " by Clang [" STRINGIFY_INT(__clang_major__) "." STRINGIFY_INT(__clang_minor__) "." STRINGIFY_INT(__clang_patchlevel__) "].";
#elif defined __GNUC__
    start = "Lanky (\"" LIGHT_BLUE LKY_VERSION_TAG DEFAULT "\"), version " LKY_VERSION_NUM ";\ncompiled " __DATE__ " for "
#ifdef __APPLE__
      "Mac OS X"
#else
      "GNU/Linux"
#endif
      " by GCC [" STRINGIFY_INT(__GNUC__) "." STRINGIFY_INT(__GNUC_MINOR__) "." STRINGIFY_INT(__GNUC_PATCHLEVEL__) "].";
#else
    start = "Lanky (\"" LKY_VERSION_TAG "\"), version " LKY_VERSION_NUM ";\ncompiled " __DATE__ " for unknown platform with unknown compiler."
#endif
    printf("%s\n" LKY_COPYRIGHT "\n", start);
    gc_init();
    arraylist list = arr_create(1);

    mach_interp interp = {NULL, get_stdlib_objects()};
    hst_put(&interp.stdlib, "Meta", stlmeta_get_class(&interp), NULL, NULL);
    
    gc_init();

    register_stdlib_prototypes();
    
    stackframe frame;
    frame.bucket = lobj_alloc();
    frame.parent_stack = list;
    frame.stack_size = 0;
    frame.locals_count = 0;
    frame.thrown = NULL;
    
    interp.stack = &frame;
    
    gc_add_func_stack(&frame);
    
    char path[1000];
    getcwd(path, 1000);

    lobj_set_member(frame.bucket, "dirname_", stlstr_cinit(path));
    
    run_repl(&interp);
    printf("\nGoodbye!\n");
}

lky_object_code *compile_from_file(char *file)
{
    yyin = fopen(file, "r");
    if(!yyin)
        return NULL;

    yyparse();
    lky_object_code *code = compile_ast_repl(programBlock->next);
    ast_free(programBlock);
    
    return code;
}

lky_object_code *render_from_file(char *file)
{
    FILE *f = fopen(file, "rb");
    lky_object_code *code = (lky_object_code *)srl_deserialize_from_file(f);
    fclose(f);
    return code;
}

void exec_from_code(lky_object_code *code, char *file, int exec)
{
    arraylist list = arr_create(1);
    mach_interp interp = {NULL};
    

    lky_object_function *func = (lky_object_function *)lobjb_build_func(code, 0, list, &interp);

    char codeloc[2000];
    realpath(file, codeloc);

    char *path = dirname(codeloc);

    interp.stdlib = get_stdlib_objects();
    hst_put(&interp.stdlib, "Meta", stlmeta_get_class(&interp), NULL, NULL);
    
    gc_init();

    register_stdlib_prototypes();

    func->bucket = lobj_alloc();
    lobj_set_member(func->bucket, "dirname_", stlstr_cinit(path));

    if(exec)
        mach_execute((lky_object_function *)func);
    else
    {
        lky_func_bundle b = MAKE_BUNDLE(NULL, lobjb_make_seq_node((lky_object *)func), &interp);
        stlmeta_examine(&b);
    }
}

int main(int argc, char *argv[])
{
    hashtable args = parse_args(argc, argv);
    if(hst_contains_key(&args, "--use-system-malloc", NULL, NULL))
        aqua_use_system_malloc_free_ = 1;
    else
        aqua_init();

    if(hst_contains_key(&args, "--no-tagged-ints", NULL, NULL))
        lobjb_uses_pointer_tags_ = 0;

    un_setup();   
    md_init();
    stlos_init(argc - 1, argv + 1);
    if(argc > 1 && !hst_contains_key(&args, argv[1], NULL, NULL))
    {
        lky_object_code *code = NULL;

        int bin = file_is_binary(argv[1]);
        if(bin)
        {
            code = render_from_file(argv[1]);
        }
        else
        {
            if(hst_contains_key(&args, "-c", NULL, NULL))
            {
                lobjb_uses_pointer_tags_ = 0;
                code = compile_from_file(argv[1]);
                size_t len;
                char *rendered = srl_serialize_object((lky_object *)code, &len);
                void (*out_func)(char *, size_t, char *) = hst_contains_key(&args, "-b", NULL, NULL) ? exp_send_to_c_source : exp_send_to_binary_file;

                out_func(rendered, len, hst_get(&args, "-o", NULL, NULL));
                free(rendered);
                goto cleanup;
            }

            code = compile_from_file(argv[1]);
        }

        if(hst_contains_key(&args, "-S", NULL, NULL))
            exec_from_code(code, argv[1], 0);
        else
            exec_from_code(code, argv[1], 1);
    }
    else
    {
        exec_in_repl();
    }

cleanup:
    pool_drain(&dlmempool);
    un_clean();
    md_unload();
    aqua_teardown();

    return 0;
}
