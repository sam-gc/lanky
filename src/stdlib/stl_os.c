#include "stl_os.h"
#include "stl_string.h"
#include "stl_array.h"

static lky_object *_stl_os_class_ = NULL;

void stlos_init(int argc, char *argv[])
{
    _stl_os_class_ = lobj_alloc();

    arraylist list = arr_create(argc + 1);
    int i;
    for(i = 0; i < argc; i++)
        arr_append(&list, stlstr_cinit(argv[i]));

    lobj_set_member(_stl_os_class_, "argc", lobjb_build_int(argc));
    lobj_set_member(_stl_os_class_, "argv", stlarr_cinit(list));
}

lky_object *stlos_get_class()
{
    return _stl_os_class_;
}
