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
#include "units.h"
#include "module.h"
#include "serialize.h"
#include "colors.h"
#include "info.h"
#include "exporter.h"

extern unsigned char lky_bottled_bytecode_data_[];

hashtable parse_args(int argc, char *argv[])
{
    hashtable tab = hst_create();
    hst_put(&tab, "-o", "a.out", NULL, NULL);

    int i;
    for(i = 0; i < argc; i++)
    {
        if(strcmp(argv[i], "-S") == 0)
            hst_put(&tab, "-S", (void *)1, NULL, NULL);
        else if(strcmp(argv[i], "--no-tagged-ints") == 0)
            hst_put(&tab, "--no-tagged-ints", (void *)1, NULL, NULL);
        else if(strcmp(argv[i], "--use-system-malloc") == 0)
            hst_put(&tab, "--use-system-malloc", (void *)1, NULL, NULL);
    }

    return tab;
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

    func->bucket = lobj_alloc();
    lobj_set_member(func->bucket, "dirname_", stlstr_cinit(path));

    if(exec)
        mach_execute((lky_object_function *)func);
    else
        stlmeta_examine(lobjb_make_seq_node((lky_object *)func), NULL);
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
    stlos_init(argc, argv);

    lky_object_code *code = (lky_object_code *)srl_deserialize_object(lky_bottled_bytecode_data_);

    if(hst_contains_key(&args, "-S", NULL, NULL))
        exec_from_code(code, argv[1], 0);
    else
        exec_from_code(code, argv[1], 1);

    pool_drain(&dlmempool);
    un_clean();
    md_unload();
    aqua_teardown();

    return 0;
}
