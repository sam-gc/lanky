// ast_compiler.c
// ================================
// This step of execution happens in several phases/passes. First a compilation environment is
// setup. That environment contains all of the information and tools we will eventually need:
// there is a collection of local variables, a set of names, a set of operations (instructions)
// and a few other misc. bits (see the 'compiler_wrapper' struct below). Once that initial
// setup is complete, we actually begin the compilation process by walking the syntax tree and
// generating the resulting instructions. In general, each ast_type (see ast.h) has its own
// compilation function called compile_<type>, though some types have multiple functions. All
// of the instructions are appended to a running list. In addition, sometimes tags are used
// when working with jumps -- we do not know when we are first compiling a loop condition
// where the end of that loop will be. Thus we add a unique tag that represents the end of the
// loop. Tags are recognizeable as they begin at 1000 (instructions are limited to the range
// [0, MAX_UCHAR]). Later, after compilation of the given unit is completed (see
// compile_compound), the tags are replaced with the index of the next operation now that it
// is known. After this is completed, the arraylist of instructions is translated to an
// unsigned character array and the resulting code object is built. This code object just
// needs to be attached to a function and then it can be executed by the interpreter.

#include "ast_compiler.h"
#include "arraylist.h"
#include "instruction_set.h"
#include "lkyobj_builtin.h"
#include "hashmap.h"
#include "bytecode_analyzer.h"
#include "stl_string.h"
#include "stl_units.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// A compiler wrapper to reduce global state.
// This struct allows us to compile in
// different contexts (for example a nested
// function).
typedef struct {
    arraylist rops; // The arraylist for the instructions (think RunningOPerationS)
    arraylist rcon; // The arraylist for the constants (think RunningCONstants)
    arraylist rnames; // The arraylist for the names (think RunningNAMES)
    arraylist loop_start_stack; // A stack for continue/jump directives
    arraylist loop_end_stack; // A stack for break directives
    Hashmap saved_locals; // A set of locals (not currently used)
    char save_val; // Used to determine if we want to save the result of an operation or pop it.
    int local_idx; // Current local slot (not currently used)
    int ifTag; // The index of the tagging system described above 
    int name_idx; // The index of the current name
    int classargc; // The number of arguments for class instantiation.
    arraylist used_names; // A list of used names (to distinguish between LI_LOAD_CLOSE and LI_LOAD_LOCAL
    int repl;
} compiler_wrapper;

typedef struct {
    compiler_wrapper *owner;
    char *name;
    long idx;
} name_wrapper;

// Forward declarations of necessary functions.
void compile(compiler_wrapper *cw, ast_node *root);
void compile_compound(compiler_wrapper *cw, ast_node *root);
void compile_single_if(compiler_wrapper *cw, ast_if_node *node, int tagOut, int tagNext);
lky_object_code *compile_ast_ext(ast_node *root, compiler_wrapper *incw);
void compile_set_member(compiler_wrapper *cw, ast_node *root);
void compile_set_index(compiler_wrapper *cw, ast_node *root);
int find_prev_name(compiler_wrapper *cw, char *name);
void int_to_byte_array(unsigned char *buffer, int val);
void append_op(compiler_wrapper *cw, long ins);

// A struct used to represent 'tags' in the
// intermediate code; we use this struct to
// collect associations between tag and
// line/instruction number
typedef struct tag_node {
    struct tag_node *next;
    long tag;
    long line;
} tag_node;

// Look up previous tags to find if there
// is a line number already found.
long get_line(tag_node *node, long tag)
{
    for(; node; node = node->next)
    {
        if(tag == node->tag)
            return node->line;
    }

    return -1;
}

// Helper function for tag system
tag_node *make_node()
{
    tag_node *node = malloc(sizeof(tag_node));
    node->next = NULL;
    node->tag = 0;
    node->line = -1;

    return node;
}

// Helper function for tag system
void append_tag(tag_node *node, long tag, long line)
{
    for(; node->next; node = node->next);

    tag_node *n = make_node();
    n->tag = tag;
    n->line = line;

    node->next = n;
}

// Helper function for tag system
void free_tag_nodes(tag_node *node)
{
    while(node)
    {
        tag_node *next = node->next;
        free(node);
        node = next;
    }
}

// Returns the index of the next local. This
// is merely used as a count for local variables.
// Currently we only use closure variables for
// simplicity's sake (at the cost of slower
// performance)
int get_next_local(compiler_wrapper *cw)
{
    return cw->local_idx++;
}

        //// Alright, we have to make this a closure.
        //int loc = w->idx;
        //compiler_wrapper *owner = w->owner;

        //if(OBJ_NUM_UNWRAP(owner->rops.items[loc]) == LI_SAVE_CLOSE)
        //    break;

        //owner->rops.items[loc] = lobjb_build_int(LI_SAVE_CLOSE);
        ////printf("HEREE\n");
        //
        //char *sid = ch;
        //char *nsid = malloc(strlen(sid) + 1);
        //strcpy(nsid, sid);

        //int idx = find_prev_name(owner, nsid);
        //if(idx < 0)
        //{
        //    idx = (int)owner->rnames.count;
        //    arr_append(&owner->rnames, nsid);
        //}

char switch_to_close(compiler_wrapper *cw, char *sid, int idx)
{
    lky_object *o = arr_get(&cw->rops, idx);
    lky_instruction istr = OBJ_NUM_UNWRAP(o);

    if(istr == LI_LOAD_CLOSE || istr == LI_SAVE_CLOSE)
        return 0;

    istr = istr == LI_LOAD_LOCAL ? LI_LOAD_CLOSE : LI_SAVE_CLOSE;

    cw->rops.items[idx] = lobjb_build_int(istr);
    char *nsid = malloc(strlen(sid) + 1);
    strcpy(nsid, sid);

    int i = find_prev_name(cw, nsid);
    if(i < 0)
    {
        i = (int)cw->rnames.count;
        arr_append(&cw->rnames, nsid);
    }

    cw->rops.items[idx + 1] = lobjb_build_int(i);
    return 1;
}

char is_close(compiler_wrapper *cw, int idx)
{
    lky_object *o = arr_get(&cw->rops, idx);
    lky_instruction istr = OBJ_NUM_UNWRAP(o);

    return istr == LI_LOAD_CLOSE || istr == LI_SAVE_CLOSE;
}

void append_var_info(compiler_wrapper *cw, char *ch, char load)
{
    char needs_close = cw->repl;
    char already_defined = 0;
    arraylist list = cw->used_names;
    int i;
    for(i = 0; i < list.count; i++)
    {
        name_wrapper *w = arr_get(&cw->used_names, i);
        
        if(strcmp(w->name, ch))
            continue;   

        if(w->owner != cw)
        {
            switch_to_close(w->owner, ch, w->idx);
            needs_close = 1;
        }
        else
        {
            needs_close = is_close(cw, w->idx);
            already_defined = 1;
        }
    }

    if(!needs_close && !already_defined && load)
        needs_close = 1;

    if(needs_close)
    {
        lky_instruction istr = load ? LI_LOAD_CLOSE : LI_SAVE_CLOSE;
        char *nsid = malloc(strlen(ch) + 1);
        strcpy(nsid, ch);

        int i = find_prev_name(cw, nsid);
        if(i < 0)
        {
            i = (int)cw->rnames.count;
            arr_append(&cw->rnames, nsid);
        }

        append_op(cw, istr);
        append_op(cw, i);

        return;
    }

    lky_instruction istr = load ? LI_LOAD_LOCAL : LI_SAVE_LOCAL;
    hm_error_t err;
    
    int idx = 0;
    lky_object_builtin *o = hm_get(&cw->saved_locals, ch, &err);
    if(err == HM_KEY_NOT_FOUND)
    {
        idx = get_next_local(cw);
        lky_object *obj = lobjb_build_int(idx);
        pool_add(&ast_memory_pool, obj);
        hm_put(&cw->saved_locals, ch, obj);
    }
    else
        idx = o->value.i;

    append_op(cw, istr);
    append_op(cw, idx);

    name_wrapper *wrap = malloc(sizeof(name_wrapper));
    pool_add(&ast_memory_pool, wrap);
    wrap->idx = cw->rops.count - 2;
    wrap->name = ch;
    wrap->owner = cw;
    arr_append(&cw->used_names, wrap);
}

// Holy actual crap..... This function is ridicilus... I'm not even going
// to try to explain it this evening. But it seems to work!!!
/*lky_instruction append_var_info(compiler_wrapper *cw, char *ch, char load)
{
    lky_instruction ret = 0;

    int i = 0;
    for(i = 0; i < cw->used_names.count; i++)
    {
        //printf("..........\n");
        name_wrapper *w = arr_get(&cw->used_names, i);
        //printf("%p\n", w);
        char *o = w->name;
        if(strcmp(o, ch))
            continue;
        
        if(w->owner == cw)
        {
            ret = load ? LI_LOAD_LOCAL : LI_SAVE_LOCAL;
            break;
        }

        ret = load ? LI_LOAD_CLOSE : LI_SAVE_CLOSE;

        // Alright, we have to make this a closure.
        int loc = w->idx;
        compiler_wrapper *owner = w->owner;

        if(OBJ_NUM_UNWRAP(owner->rops.items[loc]) == LI_SAVE_CLOSE)
            break;

        owner->rops.items[loc] = lobjb_build_int(LI_SAVE_CLOSE);
        //printf("HEREE\n");
        
        char *sid = ch;
        char *nsid = malloc(strlen(sid) + 1);
        strcpy(nsid, sid);

        int idx = find_prev_name(owner, nsid);
        if(idx < 0)
        {
            idx = (int)owner->rnames.count;
            arr_append(&owner->rnames, nsid);
        }

        owner->rops.items[loc + 1] = lobjb_build_int(idx);

    }

    if(!ret && load)
        ret = LI_LOAD_CLOSE;
    else if(!load && !ret && cw->repl)
        ret = LI_SAVE_CLOSE;
    
    if(!ret)
    {
        // If we didn't find anything, we must need to set... We will make it local.
        append_op(cw, LI_SAVE_LOCAL);
        char *sid = ch;
        hm_error_t err;
        int idx = get_next_local(cw);
        lky_object *obj = lobjb_build_int(idx);
        pool_add(&ast_memory_pool, obj);
        hm_put(&cw->saved_locals, sid, obj);
        
        append_op(cw, idx);

        name_wrapper *wrap = malloc(sizeof(name_wrapper));
        pool_add(&ast_memory_pool, wrap);
        wrap->idx = cw->rops.count - 2;
        wrap->name = ch;
        wrap->owner = cw;

        arr_append(&cw->used_names, wrap);
        //printf("HERE\n");
    }

    if(ret == LI_SAVE_LOCAL)
    {
        int idx;
        hm_error_t err;
        lky_object_builtin *o = hm_get(&cw->saved_locals, ch, &err);
        idx = o->value.i;
        append_op(cw, LI_SAVE_LOCAL);
        append_op(cw, idx);

    }

    if(ret == LI_LOAD_LOCAL)
    {
        int idx;
        hm_error_t err;
        lky_object_builtin *o = hm_get(&cw->saved_locals, ch, &err);
        idx = o->value.i;
        append_op(cw, LI_LOAD_LOCAL);
        append_op(cw, idx);
    }

    if(ret == LI_LOAD_CLOSE)
    {

        int idx = find_prev_name(cw, ch);

        if(idx < 0)
        {
            idx = (int)cw->rnames.count;
            char *ns = malloc(strlen(ch) + 1);
            strcpy(ns, ch);
            arr_append(&cw->rnames, ns);
        }

        append_op(cw, LI_LOAD_CLOSE);
        append_op(cw, idx);
    }

    if(ret == LI_SAVE_CLOSE)
    {
        append_op(cw, LI_SAVE_CLOSE);
        char *sid = ch;
        char *nsid = malloc(strlen(sid) + 1);
        strcpy(nsid, sid);

        int idx = find_prev_name(cw, nsid);
        if(idx < 0)
        {
            idx = (int)cw->rnames.count;
            arr_append(&cw->rnames, nsid);
        }

        append_op(cw, idx);
    }
} */

//             append_op(cw, (char)LI_SAVE_LOCAL);
//             char *sid = ((ast_value_node *)(node->left))->value.s;
//             hm_error_t err;
//             int idx;
//             lky_object_builtin *o = hm_get(&cw->saved_locals, sid, &err);
//             if(err == HM_KEY_NOT_FOUND)
//             {
//                 idx = get_next_local(cw);
//                 lky_object *obj = lobjb_build_int(idx);
//                 pool_add(&ast_memory_pool, obj);
//                 hm_put(&cw->saved_locals, sid, obj);
//             }
//             else
//                 idx = o->value.i;
// 
//             // printf("==> %s %d\n", sid, idx);
// 
//             append_op(cw, idx);
//             // save_val = 1;
//             return;


// Makes a lky_object from the value wrapper described
// in 'ast.h'
lky_object *wrapper_to_obj(ast_value_wrapper wrap)
{
    lky_builtin_type t;
    lky_builtin_value v;
    switch(wrap.type)
    {
    case VDOUBLE:
        t = LBI_FLOAT;
        v.d = wrap.value.d;
        break;
    case VINT:
        t = LBI_INTEGER;
        v.i = wrap.value.i;
        break;
    case VSTRING:
        {
            return stlstr_cinit(wrap.value.s);
        }
        break;
    default:
        printf("--> %d\n", wrap.type);
        return NULL;
    }

    return (lky_object *)lobjb_alloc(t, v);
}

// Helper function (boilerplate -_-)
ast_value_wrapper node_to_wrapper(ast_value_node *node)
{
    ast_value_wrapper wrap;

    wrap.type = node->value_type;
    wrap.value = node->value;

    return wrap;
}

// Converts the arraylist 'rops' referenced in 'cw'
// into an unsigned char array. Note that we are 
// downcasting from a long to an unsigned char.
// We need to make sure everything makes sense
// *before* this step.
unsigned char *finalize_ops(compiler_wrapper *cw)
{
    unsigned char *ops = malloc(cw->rops.count);

    long i;
    for(i = 0; i < cw->rops.count; i++)
    {
        lky_object_builtin *obj = arr_get(&cw->rops, i);
        ops[i] = (unsigned char)obj->value.i;
    }

    return ops;
}

// Helper function to add an instruction/tag to
// the running operations list. This should be
// the only function that appends to that list.
void append_op(compiler_wrapper *cw, long ins)
{
    lky_object *obj = lobjb_build_int(ins);
    pool_add(&ast_memory_pool, obj);
    arr_append(&cw->rops, obj);
}

arraylist copy_arraylist(arraylist in)
{
    arraylist nlist = arr_create(in.count + 10);
    memcpy(nlist.items, in.items, in.count * sizeof(void *));
    nlist.count = in.count;
    return nlist;
}

void compile_binary(compiler_wrapper *cw, ast_node *root)
{
    ast_binary_node *node = (ast_binary_node *)root;

    // We want to do something slightly different if the expression
    // is of the form 'foo.bar = baz' rather than 'bar = baz'.
    if(node->opt == '=' && node->left->type == AMEMBER_ACCESS)
    {
        // We also want to handle the special case for the 'build_'
        // function on class objects. All the below conditional
        // does is lookup the number of arguments requested by
        // the build_ function.
        char *sid = ((ast_value_node *)(node->left))->value.s;
        if(!strcmp(sid, "build_"))
        {
            ast_node *r = node->right;
            if(r->type != AFUNC_DECL) { /* TODO: Compiler error */ }
            ast_func_decl_node *n = (ast_func_decl_node *)r;
            ast_node *arg = NULL;
            int args = 0;
            for(arg = n->params; arg; arg = arg->next)
                args++;

            cw->classargc = args;
        }

        // Call the member set compiler.
        compile_set_member(cw, root);
        return;
    }
    // We also need to concern ourselves with the case of
    // 'foo[bar] = baz'
    else if(node->opt == '=' && node->left->type == AINDEX)
    {
        compile_set_index(cw, root);
        return;
    }

    // Handle the generic '=' case for compilation
    if(node->opt != '=')
        compile(cw, node->left);

    compile(cw, node->right);

    // The rest of the binary instructions are quite 
    // straightforward. We can just evaluate them in
    // a switch.
    lky_instruction istr = LI_IGNORE;
    switch(node->opt)
    {
    case '+':
        istr = LI_BINARY_ADD;
        break;
    case '-':
        istr = LI_BINARY_SUBTRACT;
        break;
    case '*':
        istr = LI_BINARY_MULTIPLY;
        break;
    case '/':
        istr = LI_BINARY_DIVIDE;
        break;
    case '%':
        istr = LI_BINARY_MODULO;
        break;
    case '^':
        istr = LI_BINARY_POWER;
        break;
    case 'l':
        istr = LI_BINARY_LT;
        break;
    case 'g':
        istr = LI_BINARY_GT;
        break;
    case 'L':
        istr = LI_BINARY_LTE;
        break;
    case 'G':
        istr = LI_BINARY_GTE;
        break;
    case 'E':
        istr = LI_BINARY_EQUAL;
        break;
    case 'n':
        istr = LI_BINARY_NE;
        break;
    case '&':
        istr = LI_BINARY_AND;
        break;
    case '|':
        istr = LI_BINARY_OR;
        break;
    case '=':
        {
            // Deal with the weirdness of the '=' case.
            append_var_info(cw, ((ast_value_node *)(node->left))->value.s, 0);
            return;

            append_op(cw, LI_SAVE_CLOSE);
            char *sid = ((ast_value_node *)(node->left))->value.s;
            char *nsid = malloc(strlen(sid) + 1);
            strcpy(nsid, sid);

            int idx = find_prev_name(cw, nsid);
            if(idx < 0)
            {
                idx = (int)cw->rnames.count;
                arr_append(&cw->rnames, nsid);
            }

            append_op(cw, idx);
            return;
        }
        break;
//        {
//             append_op(cw, (char)LI_SAVE_LOCAL);
//             char *sid = ((ast_value_node *)(node->left))->value.s;
//             hm_error_t err;
//             int idx;
//             lky_object_builtin *o = hm_get(&cw->saved_locals, sid, &err);
//             if(err == HM_KEY_NOT_FOUND)
//             {
//                 idx = get_next_local(cw);
//                 lky_object *obj = lobjb_build_int(idx);
//                 pool_add(&ast_memory_pool, obj);
//                 hm_put(&cw->saved_locals, sid, obj);
//             }
//             else
//                 idx = o->value.i;
// 
//             // printf("==> %s %d\n", sid, idx);
// 
//             append_op(cw, idx);
//             // save_val = 1;
//             return;
//         }
//         break;
    }

    append_op(cw, (char)istr);
}

// Helper function to return the next tag for the
// tag system described at the top of this file.
int next_if_tag(compiler_wrapper *cw)
{
    return cw->ifTag++;
}

void compile_loop(compiler_wrapper *cw, ast_node *root)
{
    int tagOut = next_if_tag(cw); // Prepare the exit tag
    int tagLoop = next_if_tag(cw); // Prepare the continue tag

    ast_loop_node *node = (ast_loop_node *)root;

    if(node->init) // If we have a for loop, compile the init
    {
        char save = cw->save_val;
        cw->save_val = 0;
        compile(cw, node->init);
        if(!cw->save_val)
            append_op(cw, LI_POP);
        cw->save_val = save;
    }

    int start = (int)cw->rops.count; // The start location (for loop jumps)

    compile(cw, node->condition); // Append the tag for the unknown end location
    append_op(cw, LI_JUMP_FALSE);
    append_op(cw, tagOut);
    append_op(cw, -1); // Note that we use for bytes to represent jump locations.
    append_op(cw, -1); // This allows us to index locations beyond 255 in the
    append_op(cw, -1); // interpreter.
    
    lky_object *wrapLoop = lobjb_build_int(tagLoop);
    lky_object *wrapOut = lobjb_build_int(tagOut);
    pool_add(&ast_memory_pool, wrapLoop);
    pool_add(&ast_memory_pool, wrapOut);
    arr_append(&cw->loop_start_stack, wrapLoop);
    arr_append(&cw->loop_end_stack, wrapOut);

    compile_compound(cw, node->payload->next);

    arr_remove(&cw->loop_start_stack, NULL, cw->loop_start_stack.count - 1);
    arr_remove(&cw->loop_end_stack, NULL, cw->loop_end_stack.count - 1);

    append_op(cw, tagLoop);

    if(node->onloop) // If a for loop, compile the onloop.
    {
        char save = cw->save_val;
        cw->save_val = 0;
        compile(cw, node->onloop);
        if(!cw->save_val)
            append_op(cw, LI_POP);
        cw->save_val = save;
    }

    append_op(cw, LI_JUMP); // Add the jump to the start location
    unsigned char buf[4];
    int_to_byte_array(buf, start);

    append_op(cw, buf[0]);
    append_op(cw, buf[1]);
    append_op(cw, buf[2]);
    append_op(cw, buf[3]);
    append_op(cw, tagOut);

    cw->save_val = 1;
}

// Generic break/continue compilation
void compile_one_off(compiler_wrapper *cw, ast_node *root)
{
    ast_one_off_node *node = (ast_one_off_node *)root;
    long jix = -1;

    switch(node->opt)
    {
        case 'c':
            jix = OBJ_NUM_UNWRAP(arr_get(&cw->loop_start_stack, cw->loop_start_stack.count - 1));
            break;
        case 'b':
            jix = OBJ_NUM_UNWRAP(arr_get(&cw->loop_end_stack, cw->loop_end_stack.count - 1));
            break;
    }

    append_op(cw, LI_JUMP);
    append_op(cw, jix);
    append_op(cw, -1);
    append_op(cw, -1);
    append_op(cw, -1);
}

// Generic if compilation with special cases handled in helper
// functions below.
void compile_if(compiler_wrapper *cw, ast_node *root)
{
    // Prepare tags
    int tagNext = next_if_tag(cw);
    int tagOut = next_if_tag(cw);

    ast_if_node *node = (ast_if_node *)root;

    if(!node->next_if) // If we have only one if statement
    {
        compile_single_if(cw, node, tagNext, tagNext);
        append_op(cw, tagNext);
        cw->save_val = 1;
        return;
    }

    compile_single_if(cw, node, tagOut, tagNext);

    // Compile the other if statements from if/elif/else etc.
    node = (ast_if_node *)node->next_if;
    while(node)
    {
        append_op(cw, tagNext);
        tagNext = node->next_if ? next_if_tag(cw) : tagOut;
        compile_single_if(cw, node, tagOut, tagNext);
        node = (ast_if_node *)node->next_if;
    }

    append_op(cw, tagOut);
    // printf("-%d\n", tagOut);

    cw->save_val = 1;
}

// If helper function
void compile_single_if(compiler_wrapper *cw, ast_if_node *node, int tagOut, int tagNext)
{
    if(node->condition)
    {
        compile(cw, node->condition);

        append_op(cw, LI_JUMP_FALSE);
        append_op(cw, tagNext);
        append_op(cw, -1);
        append_op(cw, -1);
        append_op(cw, -1);
        // printf("%d\n", tagNext);
    }

    compile_compound(cw, node->payload->next);

    // This is used if we want to return the last
    // line of if statements. It is broken and
    // should not be used.
    // if(arr_get(&rops, rops.count - 1) == LI_POP)
    //     arr_remove(&rops, NULL, rops.count - 1);

    if(tagOut != tagNext && node->condition)
    {
        append_op(cw, LI_JUMP);
        append_op(cw, tagOut);
        append_op(cw, -1);
        append_op(cw, -1);
        append_op(cw, -1);
    }
}

// Compiles array literals
void compile_array(compiler_wrapper *cw, ast_node *n)
{
    ast_array_node *node = (ast_array_node *)n;

    int ct = 0;
    ast_node *list = node->list;
    for(; list; list = list->next)
    {
        compile(cw, list);
        ct++;
    }

    append_op(cw, LI_MAKE_ARRAY);

    unsigned char buf[4];
    int_to_byte_array(buf, ct);

    append_op(cw, buf[0]);
    append_op(cw, buf[1]);
    append_op(cw, buf[2]);
    append_op(cw, buf[3]);
}

// Array indexing
void compile_indx(compiler_wrapper *cw, ast_node *n)
{
    ast_index_node *node = (ast_index_node *)n;

    compile(cw, node->target);
    compile(cw, node->indexer);

    append_op(cw, LI_LOAD_INDEX);
}

void compile_ternary(compiler_wrapper *cw, ast_node *n)
{
    ast_ternary_node *node = (ast_ternary_node *)n;

    int tagNext = next_if_tag(cw);
    int tagOut = next_if_tag(cw);

    // Example: a ? b : c

    // Compile 'a'
    compile(cw, node->condition);
    append_op(cw, LI_JUMP_FALSE);
    append_op(cw, tagNext);
    append_op(cw, -1);
    append_op(cw, -1);
    append_op(cw, -1);

    // Compile 'b'
    compile(cw, node->first);
    append_op(cw, LI_JUMP);
    append_op(cw, tagOut);
    append_op(cw, -1);
    append_op(cw, -1);
    append_op(cw, -1);

    // Compile 'c'
    append_op(cw, tagNext);
    compile(cw, node->second);

    // Jump out
    append_op(cw, tagOut);
}

// Used to lookup and reuse previous names/identifiers
int find_prev_name(compiler_wrapper *cw, char *name)
{
    long i;
    for(i = 0; i < cw->rnames.count; i++)
    {
        char *n = arr_get(&cw->rnames, i);
        if(strcmp(name, n) == 0)
            return (int)i;
    }

    return -1;
}

void compile_member_access(compiler_wrapper *cw, ast_node *n)
{
    ast_member_access_node *node = (ast_member_access_node *)n;

    char *name = node->ident;

    int idx = find_prev_name(cw, name);

    if(idx < 0)
    {
        idx = (int)cw->rnames.count;
        arr_append(&cw->rnames, name);
    }

    compile(cw, node->object);
    append_op(cw, LI_LOAD_MEMBER);
    append_op(cw, idx);
}

void compile_set_member(compiler_wrapper *cw, ast_node *root)
{
    ast_binary_node *bin = (ast_binary_node *)root;
    ast_member_access_node *left = (ast_member_access_node *)bin->left;
    ast_node *right = bin->right;

    compile(cw, right);

    compile(cw, left->object);

    int idx = find_prev_name(cw, left->ident);

    if(idx < 0)
    {
        idx = (int)cw->rnames.count;
        arr_append(&cw->rnames, left->ident);
    }

    append_op(cw, LI_SAVE_MEMBER);
    append_op(cw, idx);
}

void compile_set_index(compiler_wrapper *cw, ast_node *root)
{
    ast_binary_node *bin = (ast_binary_node *)root;
    ast_index_node *left = (ast_index_node *)bin->left;
    ast_node *right = bin->right;

    compile(cw, right);
    compile(cw, left->target);
    compile(cw, left->indexer);

    append_op(cw, LI_SAVE_INDEX);
}

void compile_unary(compiler_wrapper *cw, ast_node *root)
{
    ast_unary_node *node = (ast_unary_node *)root;

    // Only if there actually is a target. We use a unary
    // node for lky_nil for convenience, but that does
    // not have a target itself.
    if(node->target)
        compile(cw, node->target);

    lky_instruction istr = LI_IGNORE;
    switch(node->opt)
    {
    case 'p':
        istr = LI_PRINT;
        cw->save_val = 1;
        break;
    case 'r':
        istr = LI_RETURN;
        cw->save_val = 1;
        break;
    case '!':
        istr = LI_UNARY_NOT;
        break;
    case '0':
        istr = LI_PUSH_NIL;
        break;
    case '-':
        istr = LI_UNARY_NEGATIVE;
        break;
    }

    append_op(cw, (char)istr);
}

// Used to find and reuse previous constants.
long find_prev_const(compiler_wrapper *cw, lky_object *obj)
{
    long i;
    for(i = 0; i < cw->rcon.count; i++)
    {
        lky_object *o = arr_get(&cw->rcon, i);
        if(lobjb_quick_compare(obj, o))
            return i;
    }

    return -1;
}

void compile_var(compiler_wrapper *cw, ast_value_node *node)
{
    append_var_info(cw, node->value.s, 1); 
    return;

    int idx = find_prev_name(cw, node->value.s);

    if(idx < 0)
    {
        idx = (int)cw->rnames.count;
        char *ns = malloc(strlen(node->value.s) + 1);
        strcpy(ns, node->value.s);
        arr_append(&cw->rnames, ns);
    }

    append_op(cw, LI_LOAD_CLOSE);
    append_op(cw, idx);
//    lky_object_builtin *obj = hm_get(&cw->saved_locals, node->value.s, NULL);
//
//    int idx = obj->value.i;
//
//    append_op(cw, LI_LOAD_LOCAL);
//    append_op(cw, (char)idx);
}

// Used to compile constants and variables
void compile_value(compiler_wrapper *cw, ast_node *root)
{
    ast_value_node *node = (ast_value_node *)root;

    if(node->value_type == VVAR)
    {
        compile_var(cw, node);
        return;
    }

    lky_object *obj = wrapper_to_obj(node_to_wrapper(node));
    rc_incr(obj);

    long idx = find_prev_const(cw, obj);

    if(idx < 0)
    {
        idx = cw->rcon.count;
        arr_append(&cw->rcon, obj);
    }

    append_op(cw, LI_LOAD_CONST);
    append_op(cw, (char)idx);
}

// Compile special unit syntax
void compile_unit_value(compiler_wrapper *cw, ast_node *root)
{
    ast_unit_value_node *node = (ast_unit_value_node *)root;

    long idx = cw->rcon.count;
    arr_append(&cw->rcon, stlun_cinit(node->val, node->fmt));

    append_op(cw, LI_LOAD_CONST);
    append_op(cw, (char)idx);
}

// Compiles a function declaration
void compile_function(compiler_wrapper *cw, ast_node *root)
{
    ast_func_decl_node *node = (ast_func_decl_node *)root; 

    // We want to build a new compiler wrapper for building
    // the function in a new context.
    compiler_wrapper nw;
    nw.local_idx = 0;
    nw.saved_locals = hm_create(100, 1);
    nw.rnames = arr_create(10);
    //printf("-> %d\n", cw->used_names.count);
    nw.used_names = copy_arraylist(cw->used_names);
    nw.repl = 0;
    
    // Deal with parameters
    int argc = 0;
    ast_value_node *v = (ast_value_node *)node->params;
    for(; v; v = (ast_value_node *)v->next)
    {
        char *idf = v->value.s;

        char *nid = malloc(strlen(idf) + 1);
        strcpy(nid, idf);

        arr_append(&nw.rnames, nid);

       //  long idx = get_next_local(&nw);
       //  lky_object *obj = lobjb_build_int(idx);
       //  pool_add(&ast_memory_pool, obj);
       //  hm_put(&nw.saved_locals, v->value.s, obj);
       argc++;
    }
    
    nw.save_val = 0;
    lky_object_code *code = compile_ast_ext(node->payload->next, &nw);

    rc_incr((lky_object *)code);
    
    long idx = cw->rcon.count;
    arr_append(&cw->rcon, code);
    
    append_op(cw, LI_LOAD_CONST);
    append_op(cw, idx);
    append_op(cw, LI_MAKE_FUNCTION);
    append_op(cw, argc);
}

void compile_class_decl(compiler_wrapper *cw, ast_node *root)
{
    ast_class_decl_node *node = (ast_class_decl_node *)root;

    compiler_wrapper nw;
    nw.local_idx = 0;
    nw.saved_locals = hm_create(100, 1);
    nw.rnames = arr_create(10);
    nw.used_names = copy_arraylist(cw->used_names);

    long idx = find_prev_name(cw, node->refname);

    if(idx < 0)
    {
        idx = cw->rnames.count;
        char *nid = malloc(strlen(node->refname) + 1);
        strcpy(nid, node->refname);
        arr_append(&cw->rnames, nid);
    }

    nw.save_val = 0;
    lky_object_code *code = compile_ast_ext(node->payload->next, &nw);

    rc_incr((lky_object *)code);

    long cidx = cw->rcon.count;
    arr_append(&cw->rcon, code);

    append_op(cw, LI_LOAD_CONST);
    append_op(cw, cidx);
    append_op(cw, LI_MAKE_FUNCTION);
    append_op(cw, nw.classargc);
    //printf("%d --- \n", nw.classargc);
    append_op(cw, LI_MAKE_CLASS);
    append_op(cw, idx);
}

void compile_function_call(compiler_wrapper *cw, ast_node *root)
{
    ast_func_call_node *node = (ast_func_call_node *)root;

    ast_node *arg = node->arguments;

    int ct = 0;

    for(; arg; arg = arg->next)
    {
        compile(cw, arg);
        ct++;
    }

    compile(cw, node->ident);
    append_op(cw, LI_CALL_FUNC);
    append_op(cw, ct);

    // cw->save_val = 1;
}

// Main compiler dispatch system
void compile(compiler_wrapper *cw, ast_node *root)
{
    switch(root->type)
    {
        case ABINARY_EXPRESSION:
            compile_binary(cw, root);
        break;
        case AUNARY_EXPRESSION:
            compile_unary(cw, root);
        break;
        case AVALUE:
            compile_value(cw, root);
        break;
        case AUNIT:
            compile_unit_value(cw, root);
        break;
        case AIF:
            compile_if(cw, root);
        break;
        case ALOOP:
            compile_loop(cw, root);
        break;
        case AFUNC_DECL:
            compile_function(cw, root);
        break;
        case AFUNC_CALL:
            compile_function_call(cw, root);
        break;
        case ACLASS_DECL:
            compile_class_decl(cw, root);
        break;
        case ATERNARY:
            compile_ternary(cw, root);
        break;
        case AMEMBER_ACCESS:
            compile_member_access(cw, root);
        break;
        case AARRAY:
            compile_array(cw, root);
        break;
        case AINDEX:
            compile_indx(cw, root);
        break;
        case AONEOFF:
            compile_one_off(cw, root);
        break;
        default:
        break;
    }
}

// Compiles multi-statements/blocks of code
void compile_compound(compiler_wrapper *cw, ast_node *root)
{
    while(root)
    {
        cw->save_val = 0;
        compile(cw, root);
        if(!cw->save_val)
            append_op(cw, LI_POP);

        root = root->next;
    }
}

// TODO: We should probably worry about endienness
void int_to_byte_array(unsigned char *buffer, int val)
{
    buffer[3] = (val >> 24) & 0xFF;
    buffer[2] = (val >> 16) & 0xFF;
    buffer[1] = (val >> 8)  & 0xFF;
    buffer[0] = val         & 0xFF;
}

// As described in the explanation at the top of the file,
// we need to replace tags with and index to other code. The way this
// works is the array of operations is walked backwards. The first
// time a tag is seen, it is recorded as the final destination for
// that particular tag. Every subsquent lookup, the tag is replaced
// with the location.
void replace_tags(compiler_wrapper *cw)
{
    tag_node *tags = make_node();

    long i;
    for(i = cw->rops.count - 1; i >= 0; i--)
    {
        long op = ((lky_object_builtin *)arr_get(&cw->rops, i))->value.i;
        if(op < 0) // This should *never* happen.
            continue;

        if(op > 999) // If we are dealing with a tag...
        {
            long line = get_line(tags, op); // Get the line associated with the tag
            if(line < 0) // A negative value indicates the tag is as-yet unseen.
            {
                // Make a new tag location with the current index (i)
                append_tag(tags, op, i);

                // Replace the tag with the 'ignore' instruction (think NOP)
                lky_object *obj = lobjb_build_int(LI_IGNORE);
                pool_add(&ast_memory_pool, obj);
                cw->rops.items[i] = obj;
                continue;
            }

            // Otherwise we have a valid tag location and we can fix our jumps.
            unsigned char buf[4];
            int_to_byte_array(buf, (int)line);

            int j;
            for(j = 0; j < 4; j++)
            {   // Deal with our multiple bytes for addressing
                lky_object *obj = lobjb_build_int(buf[j]);
                pool_add(&ast_memory_pool, obj);
                cw->rops.items[i + j] = obj;
            }
        }
    }

    free_tag_nodes(tags);
}

// Finalize the constants into an array
void **make_cons_array(compiler_wrapper *cw)
{
    void **data = malloc(sizeof(void *) * cw->rcon.count);

    long i;
    for(i = 0; i < cw->rcon.count; i++)
    {
        data[i] = arr_get(&cw->rcon, i);
    }

    return data;
}

// Finalize the names into an array
char **make_names_array(compiler_wrapper *cw)
{
    char **names = malloc(sizeof(char *) * cw->rnames.count);

    long i;
    for(i = 0; i < cw->rnames.count; i++)
    {
        char *txt = arr_get(&cw->rnames, i);
        char *nw = malloc(strlen(txt) + 1);
        strcpy(nw, txt);
        names[i] = nw;
    }

    return names;
}

lky_object_code *compile_ast_repl(ast_node *root)
{
    compiler_wrapper cw;
    cw.saved_locals = hm_create(10, 1);
    cw.local_idx = 0;
    cw.rnames = arr_create(10);
    cw.used_names = arr_create(10);
    cw.repl = 1;

    return compile_ast_ext(root, &cw);
}

// The brain of the operation; compiles an AST from its root. Sometimes we want
// to set some initial settings, so we pass in a compiler wrapper. That argument
// is optional.
lky_object_code *compile_ast_ext(ast_node *root, compiler_wrapper *incw)
{
    compiler_wrapper cw;
    cw.rops = arr_create(50);
    cw.rcon = arr_create(10);
    cw.loop_start_stack = arr_create(10);
    cw.loop_end_stack = arr_create(10);
    cw.ifTag = 1000;
    cw.save_val = 0;
    cw.name_idx = 0;
    cw.classargc = 0;
    
    if(incw)
    {
        cw.saved_locals = incw->saved_locals;
        cw.local_idx = incw->local_idx;
        cw.rnames = incw->rnames;
        cw.used_names = incw->used_names;
        cw.repl = incw->repl;
    }
    else
    {
        cw.saved_locals = hm_create(100, 1);
        cw.local_idx = 0;
        cw.rnames = arr_create(50);
        cw.used_names = arr_create(20);
        cw.repl = 0;
    }

    compile_compound(&cw, root);
    replace_tags(&cw);

    // We want to propogate the classargc
    // (number of args passed to build_)
    // back up to the caller.
    if(incw)
        incw->classargc = cw.classargc;
    
    if(OBJ_NUM_UNWRAP(arr_get(&cw.rops, cw.rops.count - 1)) != LI_RETURN)
    {
        append_op(&cw, LI_PUSH_NIL);
        append_op(&cw, LI_RETURN);
    }

    // Build the resultant code object.
    lky_object_code *code = malloc(sizeof(lky_object_code));
    code->constants = make_cons_array(&cw);
    code->num_constants = cw.rcon.count;
    code->num_locals = cw.local_idx;
    code->size = sizeof(lky_object_code);
    code->mem_count = 0;
    code->type = LBI_CODE;
    code->num_names = cw.rnames.count;
    //code->members = trie_new();
    code->ops = finalize_ops(&cw);
    code->op_len = cw.rops.count;
    code->locals = malloc(sizeof(void *) * cw.local_idx);
    code->names = make_names_array(&cw);
    //code->cls = NULL;
    code->stack_size = calculate_max_stack_depth(code->ops, (int)code->op_len);

    int i;
    for(i = 0; i < cw.local_idx; i++)
        code->locals[i] = NULL;

    return code;
}

// Externally visible compilation function
lky_object_code *compile_ast(ast_node *root)
{
    return compile_ast_ext(root, NULL);
}

// Writes the code to a file (now deprecated.)
void write_to_file(char *name, lky_object_code *code)
{
    FILE *f = fopen(name, "w");

    void **cons = code->constants;
    fwrite(&(code->num_constants), sizeof(long), 1, f);
    fwrite(&(code->num_locals), sizeof(long), 1, f);
    fwrite(&(code->num_names), sizeof(long), 1, f);
    fwrite(&(code->stack_size), sizeof(long), 1, f);

    int i;
    for(i = 0; i < code->num_names; i++)
    {
        char *str = code->names[i];
        long len = strlen(str) + 1;
        fwrite(&len, sizeof(long), 1, f);
        fwrite(str, sizeof(unsigned char), len, f);
    }

    for(i = 0; i < code->num_constants; i++)
    {
        lky_object *obj = cons[i];
        lobjb_serialize(obj, f);
    }

    fwrite(&(code->op_len), sizeof(long), 1, f);

    fwrite(code->ops, sizeof(char), code->op_len, f);

    fclose(f);
}

