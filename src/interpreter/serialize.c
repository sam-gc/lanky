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

int32_t srl_bytes_to_int32(unsigned char *buf, size_t offset)
{
    int32_t bd = 0;
    int i;
    for(i = 0; i < 4; i++)
    {
        int32_t c = buf[i + offset];
        c <<= 24 - i * 8;
        bd |= c;
    }

    return bd;
}

int64_t srl_bytes_to_int64(unsigned char *buf, size_t offset)
{
    int64_t bd = 0;
    int i;
    for(i = 0; i < 4; i++)
    {
        int64_t c = buf[i + offset];
        c <<= 56 - i * 8;
    }

    return bd;
}

int32_t srl_read_int32_from_file(FILE *f)
{
    char buf[4];
    fread(buf, 1, 4, f);
    return srl_bytes_to_int32(buf, 0);
}

void srl_render_shared_info(lky_object *obj, unsigned char *buf, size_t len)
{   
    buf[0] = (char)obj->type;

    char lbts[4];
    srl_int32_to_bytes(len, lbts);

    buf[1] = lbts[0];
    buf[2] = lbts[1];
    buf[3] = lbts[2];
    buf[4] = lbts[3];
}

void srl_parse_shared_info(unsigned char *buf, lky_builtin_type *t, size_t *len)
{
    t && (*t = buf[0]);
    len && (*len = srl_bytes_to_int32(buf, 1));
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
    size_t accum = 25;

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
    srl_copy_int32_to_index(data, code->stack_size, 21);

    size_t idx = 25;
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

lky_object *srl_deserialize_number(char *bytes)
{
    char type = bytes[0];

    if(type == LBI_INTEGER)
        return lobjb_build_int(srl_bytes_to_int64(bytes, 5));
    if(type == LBI_FLOAT)
        return lobjb_build_float(*((double *)(bytes + 5)));

    return NULL;
}

lky_object *srl_deserialize_string(char *bytes)
{
    int len = srl_bytes_to_int32(bytes, 1);
    char tex[len + 1];
    memcpy(tex, bytes + 5, len);
    tex[len] = 0;

    return stlstr_cinit(tex);
}
    /*size_t accum = 21;

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

typedef struct {
    lky_builtin_type type;
    int mem_count;
    size_t size;

    long num_constants;
    long num_locals;
    long num_names;

    void **constants;
    void **locals;
    char **names;
    unsigned char *ops;
    long op_len;
    int stack_size;
} lky_object_code;*/

lky_object *srl_deserialize_code(char *bytes)
{   
    long ncs = srl_bytes_to_int32(bytes, 5);
    long nlc = srl_bytes_to_int32(bytes, 9);
    long nnm = srl_bytes_to_int32(bytes, 13);
    long nop = srl_bytes_to_int32(bytes, 17);
    long sss = srl_bytes_to_int32(bytes, 21);

    bytes += 25;

    void **cons = malloc(sizeof(void *) * ncs);
    char **names = malloc(sizeof(char *) * nnm);
    void **locs = calloc(sizeof(void *) * nlc, 1);

    int i;
    for(i = 0; i < ncs; i++)
    {
        size_t len;
        srl_parse_shared_info(bytes, NULL, &len);
        cons[i] = srl_deserialize_object(bytes);
        bytes += len;
    }

    for(i = 0; i < nnm; i++)
    {
        int len = srl_bytes_to_int32(bytes, 0);
        bytes += 4;
        names[i] = calloc(len + 1, 1);
        memcpy(names[i], bytes, len);
        bytes += len;
    }

    unsigned char *ops = calloc(nop, 1);
    memcpy(ops, bytes, nop);

    lky_object_code *code = malloc(sizeof(lky_object_code));
    code->type = LBI_CODE;
    code->size = sizeof(lky_object_code);
    code->mem_count = 0;
    code->ops = ops;
    code->constants = cons;
    code->locals = locs;
    code->names = names;
    code->op_len = nop;
    code->num_constants = ncs;
    code->num_locals = nlc;
    code->num_names = nnm;
    code->stack_size = sss;

    return (lky_object *)code;
}

lky_object *srl_deserialize_object(char *bytes)
{
    char type = bytes[0];
    switch(type)
    {
        case LBI_INTEGER:
        case LBI_FLOAT:
            return srl_deserialize_number(bytes);
        case LBI_CODE:
            return srl_deserialize_code(bytes);
        case LBI_STRING:
            return srl_deserialize_string(bytes);
    }

    return NULL;
}

lky_object *srl_deserialize_from_file(FILE *f)
{
    fseek(f, 1, 0);
    int c = srl_read_int32_from_file(f);

    fseek(f, 0, 0);

    char bytes[c];
    fread(bytes, 1, c, f);

    return srl_deserialize_object(bytes);
}
