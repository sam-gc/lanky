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

// Tutorial on recursive descent parsing:   http://matt.might.net/articles/parsing-regex-with-recursive-descent/
// Inspiration for code to build NFA:       https://swtch.com/~rsc/regexp/regexp1.html

#include "regex.h"
#include "mempool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SPLIT 256
#define MATCH 257

typedef enum {
    RAN_OR,
    RAN_SEQUENCE,
    RAN_BASE,
    RAN_REPETION,
    RAN_AT_LEAST_ONE,
    RAN_ONE_OR_NONE,
    RAN_BLANK
} rgx_ast_type;

typedef struct {
    rgx_ast_type type;
} rgx_ast_node;

typedef struct {
    rgx_ast_type type;

    rgx_ast_node *first;
    rgx_ast_node *second;
} rgx_ast_or_seq;

typedef struct {
    rgx_ast_type type;

    rgx_ast_node *loop;
} rgx_ast_repetition;

typedef struct {
    rgx_ast_type type;

    char c;
} rgx_ast_base;

struct state {
    int c;
    struct state *out;
    struct state *out_alt;
    int listit;
};

union dangling_pointers {
    struct state *state;
    union dangling_pointers *next;
};

typedef struct {
    char *input;
    char *error;

    lky_mempool mempool;
} rgx_compiler;

typedef struct state rgx_state;
typedef union dangling_pointers dangling_pointers;
typedef struct {
    rgx_state *start;
    dangling_pointers *out;
} rgx_fragment;

typedef struct {
    rgx_state **mem;
    int ct;
} rgxr_list;

struct regex {
    rgx_state *start;
    rgx_state **l1;
    rgx_state **l2;

    rgxr_list *curr_list;
    rgxr_list *next_list;

    int listgen;
    int matched;

    int state_count;
    lky_mempool state_mempool;
};

static rgx_ast_node rgx_ast_blank = {RAN_BLANK};

rgx_ast_node *rgxc_base(rgx_compiler *compiler);
rgx_ast_node *rgxc_factor(rgx_compiler *compiler);
rgx_ast_node *rgxc_term(rgx_compiler *compiler);
rgx_ast_node *rgxc_regex(rgx_compiler *compiler);
rgx_fragment rgxb_build(rgx_ast_node *head, rgx_regex *regex);
rgx_state *rgxb_build_state(int c, rgx_state *outa, rgx_state *outb, rgx_regex *regex);
void rgxb_patch(dangling_pointers *p, rgx_state *s);

rgx_compiler rgxc_make_compiler(char *input)
{
    rgx_compiler compiler;
    compiler.input = malloc(strlen(input) + 1);
    compiler.error = NULL;
    strcpy(compiler.input, input);

    compiler.mempool = pool_create();
    pool_add(&compiler.mempool, compiler.input);

    return compiler;
}

char rgxc_peek(rgx_compiler *compiler)
{
    return compiler->input[0];
}

void rgxc_eat(rgx_compiler *compiler, char expect)
{
    if(rgxc_peek(compiler) == expect)
        compiler->input++;
    else
        compiler->error = "Unexpected Input.";
}

char rgxc_next(rgx_compiler *compiler)
{
    char expect = rgxc_peek(compiler);
    rgxc_eat(compiler, expect);
    return expect;
}

char rgxc_has_more(rgx_compiler *compiler)
{
    return strlen(compiler->input) > 0;
}

void *rgxc_malloc(rgx_compiler *compiler, size_t size)
{
    void *ptr = malloc(size);
    pool_add(&compiler->mempool, ptr);

    return ptr;
}

rgx_regex *rgx_compile(char *input)
{
    rgx_compiler compiler = rgxc_make_compiler(input);

    rgx_ast_node *head = rgxc_regex(&compiler);

    rgx_regex *regex = malloc(sizeof(*regex));
    regex->state_count = 0;
    regex->listgen = 0;
    regex->state_mempool = pool_create();

    rgx_fragment frag = rgxb_build(head, regex);
    pool_drain(&compiler.mempool);

    regex->start = frag.start;

    rgx_state *match = rgxb_build_state(MATCH, NULL, NULL, regex);
    rgxb_patch(frag.out, match);

    return regex;
}

rgx_ast_node *rgxc_regex(rgx_compiler *compiler)
{
    rgx_ast_node *term = rgxc_term(compiler);

    if(rgxc_has_more(compiler) && rgxc_peek(compiler) == '|')
    {
        rgxc_eat(compiler, '|');
        rgx_ast_node *rgx = rgxc_regex(compiler);

        rgx_ast_or_seq *or = rgxc_malloc(compiler, sizeof(*or));
        or->type = RAN_OR;
        or->first = term;
        or->second = rgx;

        return (rgx_ast_node *)or;
    }

    return term;
}

rgx_ast_node *rgxc_term(rgx_compiler *compiler)
{
    rgx_ast_node *factor = &rgx_ast_blank;

    while(rgxc_has_more(compiler) && rgxc_peek(compiler) != '|' && rgxc_peek(compiler) != ')')
    {
        rgx_ast_node *next = rgxc_factor(compiler);

        rgx_ast_or_seq *seq = rgxc_malloc(compiler, sizeof(*seq));
        seq->type = RAN_SEQUENCE;
        seq->first = factor;
        seq->second = next;

        factor = (rgx_ast_node *)seq;
    }

    return factor;
}

rgx_ast_node *rgxc_factor(rgx_compiler *compiler)
{
    rgx_ast_node *base = rgxc_base(compiler);
    while(rgxc_has_more(compiler) && (rgxc_peek(compiler) == '*' || rgxc_peek(compiler) == '+' || rgxc_peek(compiler) == '?'))
    {
        char next = rgxc_peek(compiler);
        rgxc_eat(compiler, next);
        rgx_ast_repetition *node = rgxc_malloc(compiler, sizeof(*node));

        switch(next)
        {
            case '*':
                node->type = RAN_REPETION;
                break;
            case '+':
                node->type = RAN_AT_LEAST_ONE;
                break;
            case '?':
                node->type = RAN_ONE_OR_NONE;
                break;
            default:
                compiler->error = "Wut?";
                break;
        }

        node->loop = base;

        base = (rgx_ast_node *)node;
    }

    return base;
}

rgx_ast_node *rgxc_base(rgx_compiler *compiler)
{
    char c = '\0';
    switch(rgxc_peek(compiler))
    {
        case '(':
            rgxc_eat(compiler, '(');
            rgx_ast_node *r = rgxc_regex(compiler);
            rgxc_eat(compiler, ')');
            return r;

        case '\\':
            rgxc_eat(compiler, '\\');
            c = rgxc_next(compiler);
            break;

        default:
            c = rgxc_next(compiler);
            break;
    }

    rgx_ast_base *node = rgxc_malloc(compiler, sizeof(*node));
    node->type = RAN_BASE;
    node->c = c;

    return (rgx_ast_node *)node;
}

dangling_pointers *rgxb_singleton(rgx_state **state)
{
    dangling_pointers *p = (dangling_pointers *)state;
    p->next = NULL;

    return p;
}

dangling_pointers *rgxb_joined(dangling_pointers *a, dangling_pointers *b)
{
    dangling_pointers *head = a;
    for(; a->next; a = a->next);
    a->next = b;
    return head;
}

void rgxb_patch(dangling_pointers *p, rgx_state *s)
{
    while(p)
    {
        dangling_pointers *n = p->next;
        p->state = s;
        p = n;
    }
}

rgx_fragment rgxb_build_fragment(rgx_state *start, dangling_pointers *out)
{
    rgx_fragment frag;
    frag.out = out;
    frag.start = start;

    return frag;
}

rgx_state *rgxb_build_state(int c, rgx_state *outa, rgx_state *outb, rgx_regex *regex)
{
    rgx_state *state = malloc(sizeof(*state));
    state->c = c;
    state->out = outa;
    state->out_alt = outb;
    state->listit = -1;

    regex->state_count++;
    pool_add(&regex->state_mempool, state);

    return state;
}

rgx_fragment rgxb_or(rgx_ast_node *node, rgx_regex *regex)
{
    rgx_ast_or_seq *or = (rgx_ast_or_seq *)node;
    rgx_fragment left = rgxb_build(or->first, regex);
    rgx_fragment right = rgxb_build(or->second, regex);

    rgx_state *ns = rgxb_build_state(SPLIT, left.start, right.start, regex);
    return rgxb_build_fragment(ns, rgxb_joined(left.out, right.out));
}

rgx_fragment rgxb_sequence(rgx_ast_node *node, rgx_regex *regex)
{
    rgx_ast_or_seq *or = (rgx_ast_or_seq *)node;
    rgx_fragment left = rgxb_build(or->first, regex);
    rgx_fragment right = rgxb_build(or->second, regex);

    rgxb_patch(left.out, right.start);
    return rgxb_build_fragment(left.start, right.out);
}

rgx_fragment rgxb_base(rgx_ast_node *node, rgx_regex *regex)
{
    rgx_ast_base *base = (rgx_ast_base *)node;
    rgx_state *s = rgxb_build_state(base->c, NULL, NULL, regex);
    return rgxb_build_fragment(s, rgxb_singleton(&s->out));
}

rgx_fragment rgxb_repetition(rgx_ast_node *node, rgx_regex *regex)
{
    rgx_ast_repetition *rep = (rgx_ast_repetition *)node;
    rgx_fragment e = rgxb_build(rep->loop, regex);
    rgx_state *s = rgxb_build_state(SPLIT, e.start, NULL, regex);
    rgxb_patch(e.out, s);
    return rgxb_build_fragment(s, rgxb_singleton(&s->out_alt));
}

rgx_fragment rgxb_at_least_one(rgx_ast_node *node, rgx_regex *regex)
{
    rgx_ast_repetition *rep = (rgx_ast_repetition *)node;
    rgx_fragment e = rgxb_build(rep->loop, regex);
    rgx_state *s = rgxb_build_state(SPLIT, e.start, NULL, regex);
    rgxb_patch(e.out, s);
    return rgxb_build_fragment(e.start, rgxb_singleton(&s->out_alt));
}

rgx_fragment rgxb_one_or_none(rgx_ast_node *node, rgx_regex *regex)
{
    rgx_ast_repetition *rep = (rgx_ast_repetition *)node;
    rgx_fragment e = rgxb_build(rep->loop, regex);
    rgx_state *s = rgxb_build_state(SPLIT, e.start, NULL, regex);
    return rgxb_build_fragment(s, rgxb_joined(e.out, rgxb_singleton(&s->out_alt)));
}

rgx_fragment rgxb_blank(rgx_ast_node *node, rgx_regex *regex)
{
    rgx_state *s = rgxb_build_state(-1, NULL, NULL, regex);
    return rgxb_build_fragment(s, rgxb_singleton(&s->out));
}

rgx_fragment rgxb_build(rgx_ast_node *head, rgx_regex *regex)
{
    switch(head->type)
    {
        case RAN_OR:
            return rgxb_or(head, regex);
        case RAN_SEQUENCE:
            return rgxb_sequence(head, regex);
        case RAN_BASE:
            return rgxb_base(head, regex);
        case RAN_REPETION:
            return rgxb_repetition(head, regex);
        case RAN_AT_LEAST_ONE:
            return rgxb_at_least_one(head, regex);
        case RAN_ONE_OR_NONE:
            return rgxb_one_or_none(head, regex);
        case RAN_BLANK:
            return rgxb_blank(head, regex);
        default:
            return rgxb_blank(head, regex);
    }
}

void rgx_add_state(rgx_regex *regex, rgxr_list *list, rgx_state *state)
{
    if(state->listit == regex->listgen)
        return;

    state->listit = regex->listgen;
    if(state->c == SPLIT)
    {
        rgx_add_state(regex, list, state->out);
        rgx_add_state(regex, list, state->out_alt);
        return;
    }
    else if(state->c == -1)
    {
        rgx_add_state(regex, list, state->out);
        return;
    }

    list->mem[list->ct++] = state;
    if(state->c == MATCH)
        regex->matched = 1;
}

void rgx_step(rgx_regex *regex, char c)
{
    regex->listgen++;
    regex->matched = 0;
    rgxr_list *cur = regex->curr_list;
    rgxr_list *nex = regex->next_list;

    nex->ct = 0;
    int i;
    for(i = 0; i < cur->ct; i++)
    {
        rgx_state *s = cur->mem[i];
        if(s->c == c)
            rgx_add_state(regex, nex, s->out);
    }
}

int rgx_search(rgx_regex *regex, char *input)
{
    rgx_state *l1[regex->state_count];
    rgx_state *l2[regex->state_count];

    rgxr_list lista;
    rgxr_list listb;

    lista.ct = 0;
    lista.mem = l1;

    listb.ct = 0;
    listb.mem = l2;

    regex->l1 = l1;
    regex->l2 = l2;

    regex->curr_list = &lista;
    regex->next_list = &listb;

    rgx_add_state(regex, regex->curr_list, regex->start);

    int idx;
    for(idx = 0; *input; input++, idx++)
    {
        char *start = input;
        rgx_add_state(regex, regex->curr_list, regex->start);
        for(; *start; start++)
        {
            char c = start[0];
            rgx_step(regex, c);
            rgxr_list *temp = regex->curr_list; regex->curr_list = regex->next_list; regex->next_list = temp;

            if(regex->matched)
                return idx;
            if(regex->curr_list->ct == 0)
                break;
        }
    }

//    int res = rgx_list_contains_final(regex->curr_list);
//    regex->curr_list = regex->next_list = NULL;
//    regex->l1 = regex->l2 = NULL;

    return -1;
}

int rgx_matches(rgx_regex *regex, char *input)
{
    rgx_state *l1[regex->state_count];
    rgx_state *l2[regex->state_count];

    rgxr_list lista;
    rgxr_list listb;

    lista.ct = 0;
    lista.mem = l1;

    listb.ct = 0;
    listb.mem = l2;

    regex->l1 = l1;
    regex->l2 = l2;

    regex->curr_list = &lista;
    regex->next_list = &listb;

    rgx_add_state(regex, regex->curr_list, regex->start);

    for(; *input; input++)
    {
        char c = input[0];
        rgx_step(regex, c);
        rgxr_list *temp = regex->curr_list; regex->curr_list = regex->next_list; regex->next_list = temp;
    }

    int res = regex->matched;
    regex->curr_list = regex->next_list = NULL;
    regex->l1 = regex->l2 = NULL;

    return res;
}

void rgx_free(rgx_regex *regex)
{
    pool_drain(&regex->state_mempool);
    free(regex);
}
