#include "stanky.h"
#include "stl_array.h"
#include "stl_math.h"
#include "stl_io.h"
#include "stl_convert.h"
#include "stl_requisitions.h"
#include "stl_object.h"
#include "stl_time.h"
#include "stl_os.h"
#include "stl_table.h"

hashtable get_stdlib_objects()
{
    hashtable t = hst_create();
    t.duplicate_keys = 1;
    hst_put(&t, "Array", stlarr_get_class(), NULL, NULL);
    hst_put(&t, "Time", stltime_get_class(), NULL, NULL);
    hst_put(&t, "Math", stlmath_get_class(), NULL, NULL);
    hst_put(&t, "Io", stlio_get_class(), NULL, NULL);
    hst_put(&t, "Convert", stlcon_get_class(), NULL, NULL);
    hst_put(&t, "C", stlreq_get_class(), NULL, NULL);
    hst_put(&t, "Object", stlobj_get_class(), NULL, NULL);
    hst_put(&t, "OS", stlos_get_class(), NULL, NULL);
    hst_put(&t, "Table", stltab_get_class(), NULL, NULL);
    return t;
}
