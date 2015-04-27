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
    char nw[strlen(sb_) + 1];
    strcpy(nw, sb_);

    return stlstr_cinit(nw);
)

CLASS_MAKE_METHOD_EX(stlstr_split, self, char *, sb_,
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
