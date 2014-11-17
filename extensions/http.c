#include "Lanky.h"
#include <string.h>
#include <microhttpd.h>

typedef struct {
    lky_object_function *callback;
} http_info;

typedef struct {
    struct MHD_Response *res;
    struct MHD_Connection *conn;
    hashtable headers;
} http_response_info;

lky_object *http_hello(lky_object_seq *args, lky_object_function *func)
{
    lky_object *obj = LKY_FIRST_ARG(args);
    char *str = lobjb_stringify(obj);

    printf("Hello, %s!\n", str);

    free(str);

    LKY_RETURN_NIL;
}

void http_save(lky_object *o)
{
    lky_object_custom *self = LKY_CUST(o);
    http_info *data = self->data;

    gc_mark_object(LKY_OBJ(data->callback));
}

void http_free_res(lky_object *o)
{
    lky_object_custom *self = LKY_CUST(o);
    http_response_info *data = self->data;

    hst_free(&data->headers);

    MHD_destroy_response(data->res);
    free(data);
}

void http_for_each_header(void *key, void *val, void *data)
{
    char *k = (char *)key;
    char *v = (char *)val;
    struct MHD_Response *res = (struct MHD_Response *)data;

    MHD_add_response_header(res, k, v);
}

lky_object *http_add_header(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = LKY_CUST(func->owner);
    http_response_info *data = self->data;

    char *k = lobjb_stringify(LKY_FIRST_ARG(args));
    char *v = lobjb_stringify(LKY_SECOND_ARG(args));

    hst_put(&data->headers, k, v, NULL, NULL);

    LKY_RETURN_NIL;
}

lky_object *http_add_headers(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = LKY_CUST(func->owner);
    http_response_info *data = self->data;

    hashtable tab = stltab_unwrap(LKY_FIRST_ARG(args));
    
    hst_add_all_from(&data->headers, &tab, NULL, NULL);

    LKY_RETURN_NIL;
}

lky_object *http_finalize(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = LKY_CUST(func->owner);
    http_response_info *data = self->data;

    //char *txt = lobjb_stringify(lobj_get_member(LKY_OBJ(self), "content"));
    lky_object *strdata = lobj_get_member(LKY_OBJ(self), "content");
    char *txt = LKY_CUST(strdata)->data;
    long len = OBJ_NUM_UNWRAP(lobj_get_member(LKY_OBJ(strdata), "length"));
    int status = OBJ_NUM_UNWRAP(lobj_get_member(LKY_OBJ(self), "status"));
    data->res = MHD_create_response_from_buffer(len, (void *)txt, MHD_RESPMEM_PERSISTENT);
    hst_for_each(&data->headers, &http_for_each_header, data->res);

    MHD_queue_response(data->conn, status, data->res);

    LKY_RETURN_NIL;
}

static int
answer_to_connection(void *cls, struct MHD_Connection *connection,
                     const char *url, const char *method,
                     const char *version, const char *upload_data,
                     size_t *upload_data_size, void **con_cls)
{
    struct MHD_Response *response = NULL;
    int ret; 

    lky_object_custom *self = cls;
    http_info *info = self->data;

    lky_object *req = lobj_alloc();
    lobj_set_member(req, "method", stlstr_cinit((char *)method));
    lobj_set_member(req, "url", stlstr_cinit((char *)url));
    lobj_set_member(req, "version", stlstr_cinit((char *)version));

    lky_object_custom *res = lobjb_build_custom(sizeof(http_response_info));
    http_response_info *resinfo = malloc(sizeof(http_response_info));
    resinfo->res = response;
    resinfo->conn = connection;
    resinfo->headers = hst_create();
    resinfo->headers.duplicate_keys = 1;
    res->data = resinfo;
    res->freefunc = &http_free_res;
    LKY_ADD_METHOD(res, "finalize", 0, &http_finalize);
    LKY_ADD_METHOD(res, "addHeader", 2, &http_add_header);
    LKY_ADD_METHOD(res, "addHeaders", 1, &http_add_headers);
    lobj_set_member(LKY_OBJ(res), "content", stlstr_cinit(""));
    lobj_set_member(LKY_OBJ(res), "status", lobjb_build_int(418)); // Default status is "I'm a teapot" :)

    lky_object_seq *args = lobjb_make_seq_node(req);
    args->next = lobjb_make_seq_node(LKY_OBJ(res));

    lobjb_call(LKY_OBJ(info->callback), args);

    return MHD_YES;
}

lky_object *http_listen(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = LKY_CUST(func->owner);
    http_info *i = self->data;
    i->callback = LKY_FUNC(LKY_SECOND_ARG(args));
    int port = OBJ_NUM_UNWRAP(LKY_FIRST_ARG(args));

    struct MHD_Daemon *d;
    d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL, &answer_to_connection, self, MHD_OPTION_END);
    if(!d)
        return NULL;

    getchar();

    MHD_stop_daemon(d);
    LKY_RETURN_NIL;
}

// Main entry point
lky_object *http_init()
{
    lky_object_custom *obj = lobjb_build_custom(sizeof(http_info));

    LKY_ADD_METHOD(obj, "hello", 1, &http_hello);
    LKY_ADD_METHOD(obj, "listen", 2, &http_listen);
    obj->freefunc = &http_save;

    http_info *ifo = malloc(sizeof(http_info));
    obj->data = ifo;

    return LKY_OBJ(obj);
}
