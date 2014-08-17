#ifndef MACH_BINARY_OPS
#define MACH_BINARY_OPS

#include "lkyobj_builtin.h"

lky_object *lobjb_binary_add(lky_object *a, lky_object *b);
lky_object *lobjb_binary_subtract(lky_object *a, lky_object *b);
lky_object *lobjb_binary_multiply(lky_object *a, lky_object *b);
lky_object *lobjb_binary_divide(lky_object *a, lky_object *b);
lky_object *lobjb_binary_modulo(lky_object *a, lky_object *b);
lky_object *lobjb_binary_lessthan(lky_object *a, lky_object *b);
lky_object *lobjb_binary_greaterthan(lky_object *a, lky_object *b);
lky_object *lobjb_binary_equals(lky_object *a, lky_object *b);
lky_object *lobjb_binary_lessequal(lky_object *a, lky_object *b);
lky_object *lobjb_binary_greatequal(lky_object *a, lky_object *b);
lky_object *lobjb_binary_notequal(lky_object *a, lky_object *b);
lky_object *lobjb_binary_and(lky_object *a, lky_object *b);
lky_object *lobjb_binary_or(lky_object *a, lky_object *b);

#endif