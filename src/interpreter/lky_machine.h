#ifndef AST_MACHINE_H
#define AST_MACHINE_H

#include "lkyobj_builtin.h"
#include "arraylist.h"

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

#endif
