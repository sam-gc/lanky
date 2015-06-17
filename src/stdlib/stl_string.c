/* Lanky -- Scripting Language and Virtual Machine
 * Copyright (C) 2014  Sam Olsen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <string.h>
#include "stl_string.h"
#include "stl_array.h"
#include "stl_regex.h"
#include "hashtable.h"
#include "class_builder.h"

CLASS_MAKE_BLOB_FUNCTION(stlstr_blob_func, char *, str, how,
    if(how == CGC_FREE)
        free(str);
)

CLASS_MAKE_INIT(stlstr_init,
    char *str = NULL; 
    if($1)
        str = lobjb_stringify($1, interp_);
    else
        str = calloc(1, 1);

    CLASS_SET_BLOB(self_, "sb_", str, stlstr_blob_func);
    lobj_set_member(self_, "length", lobjb_build_int(strlen(str)));
)

CLASS_MAKE_METHOD(stlstr_stringify, self,
    return self;
)

CLASS_MAKE_METHOD_EX(stlstr_get_index, self, char *, sb_,
    long idx = OBJ_NUM_UNWRAP($1);
    
    char s[2];
    sprintf(s, "%c", sb_[idx]);
    
    return stlstr_cinit(s);
)

CLASS_MAKE_METHOD_EX(stlstr_hash, self, char *, sb_,
    return lobjb_build_int(hst_djb2(sb_, NULL));
)

CLASS_MAKE_METHOD_EX(stlstr_equals, self, char *, sb_,
    if(!lobj_is_of_class($1, stlstr_get_class()))
        return &lky_nil;

    char *stra = sb_;
    char *strb = CLASS_GET_BLOB($1, "sb_", char *);
    
    return LKY_TESTC_FAST(!strcmp(stra, strb));
)

CLASS_MAKE_METHOD_EX(stlstr_reverse, self, char *, sb_,
    char *str = sb_;
    size_t len = strlen(str);

    char nstr[len + 1];
    int i = 0;
    for(i = 0; i < len; i++)
    {
        nstr[i] = str[len - i - 1];
    }

    nstr[i] = 0;

    return stlstr_cinit(nstr);
)

CLASS_MAKE_METHOD_EX(stlstr_not_equals, self, char *, sb_,
    if(!lobj_is_of_class($1, stlstr_get_class()))
        return &lky_nil;

    char *stra = sb_;
    char *strb = CLASS_GET_BLOB($1, "sb_", char *);

    return LKY_TESTC_FAST(!!strcmp(stra, strb));
)

CLASS_MAKE_METHOD_EX(stlstr_greater_than, self, char *, sb_,
    if(!lobj_is_of_class($1, stlstr_get_class()))
        return &lky_nil;

    char *stra = sb_;
    char *strb = CLASS_GET_BLOB($1, "sb_", char *);

    return LKY_TESTC_FAST(strcmp(stra, strb) > 0);
)

CLASS_MAKE_METHOD_EX(stlstr_lesser_than, self, char *, sb_,
    if(!lobj_is_of_class($1, stlstr_get_class()))
        return &lky_nil;

    char *stra = sb_;
    char *strb = CLASS_GET_BLOB($1, "sb_", char *);

    return LKY_TESTC_FAST(strcmp(stra, strb) < 0);
)

CLASS_MAKE_METHOD_EX(stlstr_multiply, self, char *, sb_,
    lky_object *other = $1;

    if(!((uintptr_t)(other) & 1) && other->type != LBI_INTEGER)
    {
        // TODO: Type error
        return &lky_nil;
    }

    char *str = sb_;
    long ct = OBJ_NUM_UNWRAP(other);

    // If the count is 0, we should just
    // return the empty string.
    if(!ct)
        return stlstr_cinit("");

    int len = strlen(str);
    int targ = len * ct;

    char *nstr = malloc(len * ct + 1);
    nstr[targ] = 0;

    strcpy(nstr, str);

    long done = len;
    while(done < targ)
    {
        long n = (done <= targ - done ? done : targ - done);
        memcpy(nstr + done, nstr, n);
        done += n;
    }

    lky_object *ret = stlstr_cinit(nstr);
    free(nstr);

    return ret;
)

CLASS_MAKE_METHOD_EX(stlstr_set_index, self, char *, sb_,
    long i = OBJ_NUM_UNWRAP($1);
    
    char ch = (CLASS_GET_BLOB($2, "sb_", char *))[0];
    
    sb_[i] = ch;
    
    return &lky_nil;
)

CLASS_MAKE_METHOD_EX(stlstr_copy, self, char *, sb_,
    return stlstr_cinit(sb_);
)

lky_object *stlstr_replacing_generic(char *me, lky_object *replo, int *indcs, struct interp *interp)
{
    char *repl = NULL;
    size_t repllen = -1;
    if(OBJ_IS_NUMBER(replo) || replo->type != LBI_FUNCTION)
    {
        repl = lobjb_stringify(replo, interp);
        repllen = strlen(repl);
    }

    size_t melen = strlen(me);

    struct {
        char *ptr;
        int alloced;
        int ct;
    } builder;

    builder.ptr = calloc(melen * 2 + 1, 1);
    builder.alloced = melen * 2;
    builder.ct = 0;

    int cur = 0;

    while(*indcs > -1)
    {
        int start = *indcs;
        int len = *(++indcs);
        indcs++;

        int clen = start - cur;

        char *out = repl;
        size_t outlen = repllen;
        if(!out)
        {
            char temp[len + 1];
            memcpy(temp, me + start, len);
            temp[len] = '\0';
            lky_object *ff = lobjb_call(replo, LKY_ARGS(stlstr_cinit(temp)), interp);
            out = lobjb_stringify(ff, interp);
            outlen = strlen(out);
        }

        if(outlen + clen > builder.alloced - builder.ct)
        {
            builder.alloced += outlen + clen + melen;
            builder.ptr = realloc(builder.ptr, builder.alloced);
        }

        memcpy(builder.ptr + builder.ct, me + cur, clen);
        builder.ct += clen;

        memcpy(builder.ptr + builder.ct, out, outlen);
        builder.ct += outlen;

        cur = start + len;

        if(!repl)
            free(out);
    }

    if(melen - cur > builder.alloced - builder.ct)
    {
        builder.alloced += melen - cur + 5;
        builder.ptr = realloc(builder.ptr, builder.alloced);
    }

    memcpy(builder.ptr + builder.ct, me + cur, melen - cur);
    builder.ct += melen - cur;
    builder.ptr[builder.ct] = '\0';
    if(repl) free(repl);
    lky_object *o = stlstr_cinit(builder.ptr);
    free(builder.ptr);

    return o;
}

CLASS_MAKE_METHOD_EX(stlstr_replacing, self, char *, sb_,

    if(lobj_is_of_class($1, stlrgx_get_class()))
    {
        rgx_regex *regex = stlrgx_unwrap($1);
        int *i = rgx_collect_matches(regex, sb_);
        lky_object *o = stlstr_replacing_generic(sb_, $2, i, interp_);
        free(i);
        return o;
    }

    char *search = lobjb_stringify($1, interp_);
    char *loc, *prev;
    loc = prev = sb_;

    size_t slen = strlen(search);
    rgx_result_wrapper wrapper = rgx_wrapper_make();

    while(*loc)
    {
        loc = strstr(loc, search);
        if(!loc)
            break;    

        rgx_wrapper_append(&wrapper, loc - sb_);
        rgx_wrapper_append(&wrapper, slen);
        loc = loc + slen;
    }
    
    int *i = rgx_wrapper_finalize(&wrapper);
    lky_object *o = stlstr_replacing_generic(sb_, $2, i, interp_);
    free(i);
    free(search);
    return o;

    /*
    char *search = lobjb_stringify($1, interp_);
    char *repl = lobjb_stringify($2, interp_);
    char *loc = sb_;

    size_t repllen = strlen(repl);

    struct {
        char *ptr;
        int alloced;
        int ct;
    } builder;

    builder.ptr = calloc(strlen(loc) * 2, 1);
    builder.alloced = strlen(loc) * 2;
    builder.ct = 0;

    char *prev = loc;
    while(*loc)
    {
        loc = strstr(loc, search);

        if(!loc) break;
        if(repllen + loc - prev > builder.alloced - builder.ct)
        {
            builder.alloced += repllen + prev - loc + strlen(loc);
            builder.ptr = realloc(builder.ptr, builder.alloced);
        }

        memcpy(builder.ptr + builder.ct, prev, loc - prev);
        builder.ct += loc - prev;
        loc += strlen(search);
        prev = loc;

        memcpy(builder.ptr + builder.ct, repl, repllen);
        builder.ct += repllen;
    }

    size_t plen = strlen(prev);
    if(plen > builder.alloced - builder.ct)
    {
        builder.alloced += plen + 2;
        builder.ptr = realloc(builder.ptr, builder.alloced);
    }

    if(!!strcmp(prev, search))
    {
        memcpy(builder.ptr + builder.ct, prev, plen);
        builder.ct += plen;
    }
    builder.ptr[builder.ct] = '\0';
    lky_object *o = stlstr_cinit(builder.ptr);
    free(builder.ptr);
    free(search);
    free(repl);

    return o;
    */
)

lky_object *stlstr_split_regex(char *me, lky_object *regex)
{ 
    arraylist list = arr_create(10);
    rgx_regex *r = stlrgx_unwrap(regex);
    int *idcs = rgx_collect_matches(r, me);
    int *head = idcs;
    int idx = 0;
    for(; *idcs > -1; idcs += 2)
    {
        int top = idcs[0];
        int len = idcs[1];
        int tot = top - idx;

        char cur[tot + 1];
        int i;
        for(i = 0; i < tot; i++)
            cur[i] = me[i + idx];
        cur[tot] = '\0';

        idx += tot + len;
        arr_append(&list, stlstr_cinit(cur));
    }

    if(head != idcs)
    {
        idcs -= 2;
        int srest = idcs[0] + idcs[1];
        me += srest;
        arr_append(&list, stlstr_cinit(me));
    }
    else
    {
        arr_append(&list, stlstr_cinit(me));
    }

    free(head);
    return stlarr_cinit(list);
}

CLASS_MAKE_METHOD_EX(stlstr_split, self, char *, sb_,
    if(!$1)
    {
        arraylist list = arr_create(10);
        arr_append(&list, self);
        return stlarr_cinit(list);
    }

    if(lobj_is_of_class($1, stlrgx_get_class()))
        return stlstr_split_regex(sb_, $1);
        
    lky_object_function *strf = (lky_object_function *)lobj_get_member($1, "stringify_");
    if(!strf)
    {
        // TODO: Error
    }
    
    lky_func_bundle b = MAKE_BUNDLE(strf, NULL, interp_);
    lky_object *ostr = (lky_object *)(strf->callable.function)(&b);

    if(!lobj_is_of_class(ostr, stlstr_get_class()))
        return &lky_nil;

    char *delim = CLASS_GET_BLOB(ostr, "sb_", char *);

    char *loc = sb_;
    size_t delen = strlen(delim);
    
    arraylist list = arr_create(10);

    if(delen == 0)
    {
        while(*loc)
        {
            char nc[2];
            nc[0] = *loc;
            nc[1] = '\0';
            arr_append(&list, stlstr_cinit(nc));
            loc++;
        }

        return stlarr_cinit(list);
    }
    
    while(loc)
    {
        char *next = strstr(loc, delim);
        size_t nlen = (next ? next : strlen(loc) + loc) - loc + 1;
        char *str = malloc(nlen);
        memcpy(str, loc, nlen - 1);
        str[nlen - 1] = '\0';
        
        arr_append(&list, stlstr_cinit(str));
        free(str);
        
        loc = next ? next + delen : NULL;
    }
    
    return stlarr_cinit(list);
)

CLASS_MAKE_METHOD_EX(stlstr_iterable, self, char *, sb_,
    char *mestr = sb_;
    int i;
    size_t len = strlen(mestr);
    arraylist list = arr_create(len + 1);
    
    for(i = 0; i < len; i++)
    {
        char c[2];
        c[0] = mestr[i];
        c[1] = '\0';

        arr_append(&list, stlstr_cinit(c));
    }

    return stlarr_cinit(list);
)

CLASS_MAKE_METHOD_EX(stlstr_add, self, char *, sb_,
    char *mestr = sb_;
    
    lky_object *other = $1;
    char sbf = OBJ_NUM_UNWRAP($2);
    
    char *chr = lobjb_stringify(other, interp_);
    
    size_t len = strlen(chr) + strlen(mestr) + 1;
    char *newstr = malloc(len);
    
    char *first = sbf ? mestr : chr;
    char *second = first == mestr ? chr : mestr;
    
    strcpy(newstr, first);
    strcat(newstr, second);
    
    lky_object *ret = stlstr_cinit(newstr);
    free(newstr);
    free(chr);
    
    return ret;
)

char stlstr_escape_for(char i)
{
    switch(i)
    {
        case 'n':
            return '\n';
        case 't':
            return '\t';
        case '\'':
            return '\'';
        case '"':
            return '"';
        case '\\':
            return '\\';
        default:
            return -1;
    }
}

char *stlstr_copy_and_escape(char *str)
{
    unsigned long len = strlen(str);
    unsigned long i, o;

    char *cop = calloc(len + 1, sizeof(char));
    for(i = o = 0; i < len; ++i, ++o)
    {
        if(str[i] != '\\')
        {
            cop[o] = str[i];
            continue;
        }

        char e = stlstr_escape_for(str[i + 1]);
        if(e > 1)
        {
            i++;
            cop[o] = e;
        }
        else
            cop[o] = str[i];
    }

    return cop;
}

CLASS_MAKE_METHOD_EX(stlstr_to_lower, self, char *, sb_,
    char *me = sb_;
    size_t len = strlen(me);
    char n[len + 1];
    int i;
    for(i = 0; i < len; i++)
    {
        char c = me[i];
        if(c >= 'A' && c <= 'Z')
            c = (c - 'A') + 'a';

        n[i] = c;
    }

    n[i] = '\0';

    return stlstr_cinit(n);
)

CLASS_MAKE_METHOD_EX(stlstr_to_upper, self, char *, sb_,
    char *me = sb_;
    size_t len = strlen(me);
    char n[len + 1];
    int i;
    for(i = 0; i < len; i++)
    {
        char c = me[i];
        if(c >= 'a' && c <= 'z')
            c = (c - 'a') + 'A';

        n[i] = c;
    }

    n[i] = '\0';

    return stlstr_cinit(n);
)

CLASS_MAKE_METHOD_EX(stlstr_find, self, char *, sb_,
    char *me = sb_;
    if(!$1) return &lky_nil;
    CLASS_ERROR_ASSERT(lobj_is_of_class($1, stlrgx_get_class()), "MismatchedType", "Parameter 1 to find not regular expression");

    rgx_regex *regex = stlrgx_unwrap($1);
    int *pts = rgx_collect_matches(regex, me);
    int *head = pts;

    if(rgx_get_flags(regex) & RGX_GLOBAL)
    {
        arraylist list = arr_create(10);

        while(*pts > -1)
        {
            int idx = *pts;
            int len = *(++pts);
            char temp[len + 1];
            memcpy(temp, me + idx, len);
            temp[len] = '\0';
            arr_append(&list, stlstr_cinit(temp));
            
            pts++;
        }

        free(head);
        return stlarr_cinit(list);
    }
    else
    {
        int idx = *pts;
        if(idx == -1)
        {
            free(head);
            return &lky_nil;
        }

        int len = pts[1];
        char temp[len + 1];
        memcpy(temp, me + idx, len);
        temp[len] = '\0';

        free(head);
        return stlstr_cinit(temp);
    }
)

void stlstr_manual_init(lky_object *nobj, lky_object *cls, void *data)
{
    char *copied = stlstr_copy_and_escape((char *)data);
    CLASS_SET_BLOB(nobj, "sb_", copied, stlstr_blob_func);
    lobj_set_member(nobj, "length", lobjb_build_int(strlen(copied)));
}

lky_object *stlstr_cinit(char *str)
{
    return clb_instantiate(stlstr_get_class(), stlstr_manual_init, str);
}

char *stlstr_unwrap(lky_object *o)
{
    return CLASS_GET_BLOB(o, "sb_", char *);
}

static lky_object *stlstr_class_ = NULL;
lky_object *stlstr_get_class()
{
    if(stlstr_class_)
        return stlstr_class_;

    char *proto_bl = calloc(1, 1);
    lky_object *proto_blob = lobjb_build_blob(proto_bl, stlstr_blob_func);

    CLASS_MAKE(cls, NULL, stlstr_init, 1,
        CLASS_PROTO("length", lobjb_build_int(-1));
        CLASS_PROTO("sb_", proto_blob);
        CLASS_PROTO_METHOD("reverse", stlstr_reverse, 0);
        CLASS_PROTO_METHOD("find", stlstr_find, 1);
        CLASS_PROTO_METHOD("stringify_", stlstr_stringify, 0);
        CLASS_PROTO_METHOD("split", stlstr_split, 1);
        CLASS_PROTO_METHOD("replacing", stlstr_replacing, 2);
        CLASS_PROTO_METHOD("lower", stlstr_to_lower, 0);
        CLASS_PROTO_METHOD("upper", stlstr_to_upper, 0);
        CLASS_PROTO_METHOD("copy", stlstr_copy, 0);
        CLASS_PROTO_METHOD("op_get_index_", stlstr_get_index, 1);
        CLASS_PROTO_METHOD("op_set_index_", stlstr_set_index, 2);
        CLASS_PROTO_METHOD("op_equals_", stlstr_equals, 1);
        CLASS_PROTO_METHOD("op_add_", stlstr_add, 2);
        CLASS_PROTO_METHOD("op_notequal_", stlstr_not_equals, 1);
        CLASS_PROTO_METHOD("op_gt_", stlstr_greater_than, 1);
        CLASS_PROTO_METHOD("op_lt_", stlstr_lesser_than, 1);
        CLASS_PROTO_METHOD("op_multiply_", stlstr_multiply, 1);
        CLASS_PROTO_METHOD("hash_", stlstr_hash, 0);
        CLASS_PROTO_METHOD("iterable_", stlstr_iterable, 0);
    );

    stlstr_class_ = cls;
    return cls;
}
