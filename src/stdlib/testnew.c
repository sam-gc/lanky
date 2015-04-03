#include "testnew.h"
#include "class_builder.h"
#include "stl_string.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int x;
    int y;
} tn_blob;

CLASS_MAKE_BLOB_FUNCTION(tn_blob_destruct, tn_blob *, b, {
    printf("Freeing blob...\n");
    free(b);
})

CLASS_MAKE_INIT(tn_init, {
    lobj_set_member(self_, "name", $1);
    
    tn_blob *b = malloc(sizeof(*b));
    b->x = 11;
    b->y = 22;

    CLASS_SET_BLOB(self_, "tn_blob_", b, tn_blob_destruct, NULL);
})

// The above replaces:
/*
lky_object *tn_init(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);
    lky_object *me = (lky_object *)args->value;
    lky_object *name = (lky_object *)args->next->value;

    lobj_set_member(me, "name", name);

    return &lky_nil;
}
*/

CLASS_MAKE_METHOD(tn_test1, self, {
    return lobjb_build_int(5);
});

lky_object *tn_test2(lky_func_bundle *bundle)
{
    
    return lobjb_build_int(6);
    
}

CLASS_MAKE_METHOD(tn_name_me, cls, {
    printf("TN TEST\n");
})

CLASS_MAKE_METHOD(tn_stringify, self, {
    char name[100];
    sprintf(name, "(TN obj | %p)", self);
    return stlstr_cinit(name);
})

CLASS_MAKE_METHOD_EX(tn_print_blob_ifo, self, tn_blob *, tn_blob_, {
    printf("<%d, %d>\n", tn_blob_->x, tn_blob_->y);
})

CLASS_MAKE_METHOD(tn_on_destroy, self, {
    printf("Destroying TN...\n");
})

lky_object *tn_get_class()
{
    CLASS_MAKE(cls, NULL, tn_init, 1, {
        CLASS_PROTO("test", lobjb_build_int(5));
        CLASS_PROTO("name", &lky_nil);
        CLASS_PROTO_METHOD("test1", tn_test1, 0);
        CLASS_PROTO_METHOD("test2", tn_test2, 0);
        CLASS_PROTO_METHOD("stringify_", tn_stringify, 0);
        CLASS_PROTO_METHOD("printBlob", tn_print_blob_ifo, 0);
        CLASS_PROTO_METHOD("on_destroy_", tn_on_destroy, 0);
        CLASS_STATIC_METHOD("name_me", tn_name_me, 0);
    });

    return cls;
}
