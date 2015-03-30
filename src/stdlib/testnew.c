#include "testnew.h"
#include "class_builder.h"
#include "stl_string.h"

lky_object *tn_init(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);
    lky_object *me = args->value;
    lky_object *name = args->next->value;

    lobj_set_member(me, "name", name);

    return &lky_nil;
}

CLASS_MAKE_PROTO_METHOD(tn_stringify, {
    char name[100];
    sprintf(name, "(TN obj | %p)", self_);
    return stlstr_cinit(name);
})

lky_object *tn_get_class()
{
    CLASS_MAKE(cls, NULL, tn_init, 1, {
        CLASS_PROTO("test", lobjb_build_int(5));
        CLASS_PROTO("name", &lky_nil);
        CLASS_PROTO_METHOD("stringify_", tn_stringify, 0);
    });

    return cls;
}
