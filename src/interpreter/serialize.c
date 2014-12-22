#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "stl_string.h"
#include "serialize.h"

void srl_int32_to_bytes(int32_t i, char *buf)
{
    buf[0] = (i >> 24) & 0xFF;
    buf[1] = (i >> 16) & 0xFF;
    buf[2] = (i >> 8)  & 0xFF;
    buf[3] = i         & 0xFF;
}

void srl_int64_to_bytes(int64_t i, char *buf)
{
    buf[0] = (i >> 56) & 0xFF;
    buf[1] = (i >> 48) & 0xFF;
    buf[2] = (i >> 40) & 0xFF;
    buf[3] = (i >> 32) & 0xFF;
    buf[4] = (i >> 24) & 0xFF;
    buf[5] = (i >> 16) & 0xFF;
    buf[6] = (i >> 8)  & 0xFF;
    buf[7] = i         & 0xFF;
}

void srl_render_shared_info(lky_object *obj, char *buf, size_t len)
{   
    buf[0] = (char)obj->type;

    char lbts[4];
    srl_int32_to_bytes(len, lbts);

    buf[1] = lbts[0];
    buf[2] = lbts[1];
    buf[3] = lbts[2];
    buf[4] = lbts[3];
}

char *srl_serialize_number(lky_object *obj, size_t *len)
{
    char *data = malloc(13);
    char buf[8];
    *len = 14;

    srl_render_shared_info(obj, data, *len);

    if(obj->type == LBI_INTEGER)
    {
        long val = OBJ_NUM_UNWRAP(obj);
        srl_int64_to_bytes(val, buf);   
    }
    else if(obj->type == LBI_FLOAT)
    {
        double val = OBJ_NUM_UNWRAP(obj);
        char *vall = (char *)(&val);
        memcpy(buf, vall, 8);
    }

    int i;
    for(i = 0; i < 8; i++)
        data[i + 5] = buf[i];

    return data;
}

char *srl_serialize_string(lky_object *obj, size_t *len)
{
    char *tex = lobjb_stringify(obj);    
    *len = 5 + strlen(tex);
    char *data = malloc(*len);

    srl_render_shared_info(obj, data, *len);
    data[0] = (char)LBI_STRING; // Standard library strings normally have type 'LBI_CUSTOM'

    int i;
    for(i = 0; i < *len - 5; i++)
        data[i + 5] = tex[i];

    free(tex);

    return data;
}

void srl_copy_bytes_to_index(char *buf, char *src, size_t idx, size_t len)
{
    int i; 
    for(i = 0; i < len; i++)
        buf[i + idx] = src[i];
}

void srl_copy_int32_to_index(char *buf, int src, size_t idx)
{
    char tmp[4];
    srl_int32_to_bytes(src, tmp);

    srl_copy_bytes_to_index(buf, tmp, idx, 4);
}

char *srl_serialize_code(lky_object *obj, size_t *len)
{
    size_t accum = 21;

    lky_object_code *code = (lky_object_code *)obj;
    char *rendered_constants[code->num_constants];
    size_t rendered_lengths[code->num_constants];

    int i;
    for(i = 0; i < code->num_constants; i++)
    {
        size_t tmp;
        rendered_constants[i] = srl_serialize_object(code->constants[i], &tmp);
        rendered_lengths[i] = tmp;

        accum += tmp;
    }

    for(i = 0; i < code->num_names; i++)
        accum += strlen(code->names[i]) + 4;

    accum += code->op_len;
    char *data = malloc(accum);
    srl_copy_int32_to_index(data, code->num_constants, 5);
    srl_copy_int32_to_index(data, code->num_locals, 9);
    srl_copy_int32_to_index(data, code->num_names, 13);
    srl_copy_int32_to_index(data, code->op_len, 17);

    size_t idx = 21;
    for(i = 0; i < code->num_constants; i++)
    {
        char *cons = rendered_constants[i];
        size_t ln = rendered_lengths[i];
        srl_copy_bytes_to_index(data, cons, idx, ln);
        idx += ln;
        free(cons);
    }

    for(i = 0; i < code->num_names; i++)
    {
        char *cur = code->names[i];
        size_t ln = strlen(cur);
        srl_copy_int32_to_index(data, ln, idx);
        idx += 4;
        srl_copy_bytes_to_index(data, cur, idx, ln);
        idx += ln;
    }

    srl_copy_bytes_to_index(data, code->ops, idx, code->op_len);

    *len = accum;
    srl_render_shared_info(obj, data, accum);

    return data;
}

char *srl_serialize_object(lky_object *obj, size_t *len)
{   
    size_t throwaway;
    size_t *targ_len = len ? len : &throwaway;

    switch(obj->type)
    {
        case LBI_INTEGER:
        case LBI_FLOAT:
            return srl_serialize_number(obj, targ_len);
        case LBI_CODE:
            return srl_serialize_code(obj, targ_len);
        case LBI_CUSTOM_EX:
            return srl_serialize_string(obj, targ_len);
    }

    printf("%d\n", obj->type);
    return NULL;
}

lky_object *srl_deserialize_object(char *bytes)
{
    // TODO: Implement this.
    return NULL;
}
