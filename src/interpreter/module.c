#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "module.h"
#include "ast.h"
#include "parser.h"
#include "tools.h"
#include "ast_compiler.h"
#include "stl_meta.h"
#include "stanky.h"
#include "ast.h"
#include "lky_gc.h"
#include "hashtable.h"

#define YY_BUF_SIZE 16384
extern ast_node *programBlock;
typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern YY_BUFFER_STATE yy_create_buffer(FILE *, size_t);
extern void yypush_buffer_state(YY_BUFFER_STATE);
extern void yypop_buffer_state();
extern char yyyhad_error;
static hashtable interpreters;

long md_hash_interp(void *key, void *data)
{
    return (long)key;
}

int md_equ_interp(void *key, void *data)
{
    return key == data;
}

void md_free_subtable(void *key, void *val, void *data)
{
    hashtable *hst = (hashtable *)val;
    hst_free(hst);
    free(hst);
}

void md_mark_objects(void *key, void *val, void *data)
{
    gc_mark_object((lky_object *)val);
}

void md_mark_things_in_table(void *key, void *val, void *data)
{
    hashtable *hst = (hashtable *)val;
    hst_for_each(hst, md_mark_objects, NULL);
}

void md_init()
{
    interpreters = hst_create();
}

void md_unload()
{
    hst_for_each(&interpreters, md_free_subtable, NULL);
    hst_free(&interpreters);
}

void md_gc_cycle()
{
    hst_for_each(&interpreters, md_mark_things_in_table, NULL);
}

hashtable *md_active_modules_for_interp(mach_interp *ip)
{
    hashtable *hst = hst_get(&interpreters, ip, md_hash_interp, md_equ_interp);
    if(!hst)
    {
        hashtable temp = hst_create();
        temp.duplicate_keys = 1;

        hst = malloc(sizeof(hashtable));
        memcpy(hst, &temp, sizeof(hashtable));
        hst_put(&interpreters, ip, hst, md_hash_interp, md_equ_interp);
    }

    return hst;
}

char *md_get_full_filename(char *filename, char *buf)
{
    // TODO: Actually implement this function
    strcpy(buf, filename);
    return buf;
}

lky_object *md_load(char *filename, mach_interp *ip)
{   
    hashtable *hst = md_active_modules_for_interp(ip);
    char fullname[1000]; // TODO: Use the defined max-path constant

    md_get_full_filename(filename, fullname);

    lky_object *ret = hst_get(hst, fullname, NULL, NULL);

    if(ret)
        return ret;

    FILE *yyin = fopen(filename, "r");
    if(!yyin)
        return NULL;

    YY_BUFFER_STATE buffer = yy_create_buffer(yyin, YY_BUF_SIZE);
    yypush_buffer_state(buffer);

    yyparse();

    gc_pause();
    lky_object_code *code = compile_ast_repl(programBlock->next);
    ast_free(programBlock);
    gc_resume();

    yypop_buffer_state();

    arraylist list = arr_create(1);

    lky_object_function *func = (lky_object_function *)lobjb_build_func(code, 0, list, ip);

    func->bucket = lobj_alloc();
    func->bucket->members = get_stdlib_objects();
    hst_put(&func->bucket->members, "Meta", stlmeta_get_class(ip), NULL, NULL);

    ret = mach_execute((lky_object_function *)func);

    fclose(yyin);

    hst_put(hst, fullname, ret, NULL, NULL);

    return ret;
}
