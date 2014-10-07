#ifndef STL_UNITS_H
#define STL_UNITS_H

#include "lkyobj_builtin.h"
#include "units.h"

typedef struct {
    un_unit u;
} stlun_data;

lky_object *stlun_get_class();
lky_object *stlun_cinit_ex(un_unit u);
lky_object *stlun_cinit(double val, char *fmt);

#endif
