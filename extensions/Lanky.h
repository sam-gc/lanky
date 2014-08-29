#ifndef LANKY_H
#define LANKY_H

#include <stdio.h>
#include <stdlib.h>

// =================================================
// arraylist.h
// =================================================

typedef struct {
    void **items;
    long count;
    long allocated;
} arraylist;

typedef char(*arr_pointer_function)(void *);

arraylist arr_create(long count);
void arr_append(arraylist *list, void *item);
void arr_insert(arraylist *list, void *item, long idx);
void *arr_get(arraylist *list, long idx);
void arr_set(arraylist *list, void *item, long idx);
void arr_remove(arraylist *list, void *item, long idx);
long arr_length(arraylist *list);
long arr_index_of(arraylist *list, void *obj);
void arr_for_each(arraylist *list, arr_pointer_function callback);
void arr_free(arraylist *list);

// =================================================
// trie.h
// =================================================

struct TrieNode;

typedef void(*trie_pointer_function)(void *);

typedef struct list_node {
    struct list_node *next;
    struct TrieNode *payload;
} list_node_t;

typedef struct TrieNode {
    list_node_t *children;
    char value;
    void *object;
} TrieNode_t;

typedef struct {
    TrieNode_t *head;
    trie_pointer_function free_func;
    long count;
} Trie_t;

Trie_t trie_new();
void trie_free(Trie_t t);
void trie_add(Trie_t *t, char *str, void *value);
void *trie_get(Trie_t *t, char *str);
void trie_for_each(Trie_t *t, trie_pointer_function callback);
char trie_contains_path(Trie_t *t, char *str);

// =================================================
// lky_object.h
// =================================================

struct lky_object_seq;
struct lky_object;

typedef enum {
    LBI_FLOAT,
    LBI_INTEGER,
    LBI_STRING,
    LBI_SEQUENCE,
    LBI_NIL,
    LBI_FUNCTION,
    LBI_CLASS,
    LBI_CODE,
    LBI_CUSTOM,
    LBI_CUSTOM_EX,
    LBI_ERROR
} lky_builtin_type;

typedef struct lky_object *(*lky_function_ptr)(struct lky_object_seq *args, struct lky_object *self);

typedef struct {
    int argc;
    lky_function_ptr function;
} lky_callable;

typedef struct lky_object_seq {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    struct lky_object *cls;

    lky_callable callable;

    struct lky_object *value;
    struct lky_object_seq *next;
} lky_object_seq;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    struct lky_object *cls;

    lky_callable callable;
} lky_object;

lky_object *lobj_alloc();
void lobj_set_member(lky_object *obj, char *member, lky_object *val);
lky_object *lobj_get_member(lky_object *obj, char *member);
void rc_decr(lky_object *obj);
void rc_incr(lky_object *obj);
void lobj_dealloc(lky_object *obj);
void print_alloced();

extern lky_object lky_nil;

// =================================================
// lky_machine.h
// =================================================
struct interp;

typedef struct stackframe {
    struct stackframe *next;
    struct stackframe *prev;
    
    arraylist parent_stack;
    lky_object *bucket;
    void **constants;
    void **locals;
    void **data_stack;
    char **names;
    long pc;
    unsigned char *ops;
    long tape_len;
    
    struct interp *interp;
    
    long stack_pointer;
    long stack_size;
    lky_object *ret;
} stackframe;

typedef struct interp {
    stackframe *stack;
    
} mach_interp;

typedef struct lky_object_function lky_object_function;

mach_interp mach_make_interp();

lky_object *mach_interrupt_exec(lky_object_function *func);
lky_object *mach_execute(lky_object_function *func);
void mach_halt_with_err(lky_object *err);
void print_ops(char *ops, int tape_len);

// =================================================
// lobj_builtin.h
// =================================================

#define OBJ_NUM_UNWRAP(obj) (((lky_object_builtin *)obj)->type == LBI_FLOAT ? ((lky_object_builtin *)obj)->value.d : ((lky_object_builtin *)obj)->value.i)
#define BIN_ARGS lky_object *a, lky_object *b
#define BI_CAST(o, n) lky_object_builtin * n = (lky_object_builtin *) o

typedef void(*lobjb_custom_ex_dealloc_function)(lky_object *o);
typedef void(*lobjb_gc_save_function)(lky_object *o);

typedef union {
    double d;
    long i;
    char *s;
} lky_builtin_value;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *cls;

    lky_callable callable;

    lky_builtin_value value;
} lky_object_builtin;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *cls;

    lky_callable callable;

    void *data;
    lobjb_custom_ex_dealloc_function freefunc;
    lobjb_gc_save_function savefunc;
} lky_object_custom;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *cls;

    lky_callable callable;

    long num_constants;
    long num_locals;
    long num_names;

    void **constants;
    void **locals;
    char **names;
    unsigned char *ops;
    long op_len;
    int stack_size;
} lky_object_code;

// TODO: Is this necessary?
typedef struct {
    lky_object *member;
    char *name;
} lky_class_member_wrapper;

struct lky_object_function {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *cls;

    lky_callable callable;

    arraylist parent_stack;
    lky_object *bucket;
    
    mach_interp *interp;

    lky_object_code *code;
    lky_object *owner;
};

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *cls;

    lky_callable callable;

    lky_object_function *builder;
    char *refname;
} lky_object_class;

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;
    Trie_t members;
    lky_object *cls;

    char *name;
    char *text;
} lky_object_error;

lky_object *lobjb_build_int(long value);
lky_object *lobjb_build_float(double value);
lky_object *lobjb_build_error(char *name, char *text);
lky_object_custom *lobjb_build_custom(size_t extra_size);
lky_object *lobjb_build_func(lky_object_code *code, int argc, arraylist inherited, mach_interp *interp);
lky_object *lobjb_build_func_ex(lky_object *owner, int argc, lky_function_ptr ptr);
lky_object *lobjb_build_class(lky_object_function *builder, char *refname);
lky_object *lobjb_alloc(lky_builtin_type t, lky_builtin_value v);
lky_object *lobjb_default_callable(lky_object_seq *args, lky_object *self);
lky_object *lobjb_default_class_callable(lky_object_seq *args, lky_object *self);

lky_object *lobjb_unary_load_index(lky_object *obj, lky_object *indexer);
lky_object *lobjb_unary_save_index(lky_object *obj, lky_object *indexer, lky_object *newobj);

lky_object_seq *lobjb_make_seq_node(lky_object *value);
void lobjb_free_seq(lky_object_seq *seq);

void lobjb_print_object(lky_object *a);
void lobjb_print(lky_object *a);
char lobjb_quick_compare(lky_object *a, lky_object *b);
lky_object *lobjb_num_to_string(lky_object *a);

lky_object_code *lobjb_load_file(char *name);

void lobjb_serialize(lky_object *obj, FILE *f);
lky_object *lobjb_deserialize(FILE *f);

void lobjb_clean(lky_object *a);

// =================================================
// lky_gc.h
// =================================================

void gc_init();
void gc_pause();
void gc_resume();
void gc_add_func_stack(stackframe *frame);
void gc_add_root_object(lky_object *obj);
void gc_remove_root_object(lky_object *obj);
void gc_add_object(lky_object *obj);
void gc_gc();
void gc_mark_object(lky_object *o);

// =================================================
// STL Functions
// =================================================
lky_object *stlstr_cinit(char *str); // Inits a stl string
lky_object *stlarr_cinit(arraylist list); // Inits a stl array

// =================================================
// Helper Macros
// =================================================
#define LKY_CUST(obj) ((lky_object_custom *)obj)
#define LKY_OBJ(obj) ((lky_object *)obj)
#define LKY_OBJ_STRUCT(obj) ((struct lky_object *)obj)
#define LKY_FUNC(obj) ((lky_object_function *)obj)
#define LKY_CFUNC(ptr) ((lky_function_ptr)ptr)
#define LKY_FREE_FUNC(ptr) ((lobjb_custom_ex_dealloc_function)ptr)
#define LKY_GC_FUNC(ptr) ((lobjb_gc_save_function)ptr)
#define LKY_BUILTIN(obj) ((lky_object_builtin *)obj)
#define LKY_CODE(obj) ((lky_object_code *)obj)

#define LKY_FIRST_ARG(seq) (seq->value)
#define LKY_SECOND_ARG(seq) (seq->next->value)
#define LKY_THIRD_ARG(seq) (seq->next->next->value)
#define LKY_RETURN_NIL return &lky_nil

#define LKY_ADD_METHOD(obj, name, argc, ptr) (lobj_set_member((lky_object *)obj, name, lobjb_build_func_ex((lky_object *)obj, argc, (lky_function_ptr)ptr)))

#endif
