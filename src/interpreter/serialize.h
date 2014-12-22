#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "lky_object.h"
#include "lkyobj_builtin.h"

char *srl_serialize_object(lky_object *obj, size_t *len);
lky_object *srl_deserialize_object(char *bytes);

#endif
