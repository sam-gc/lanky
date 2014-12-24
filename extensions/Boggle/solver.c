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

// typedef struct _lnode {
//     void *value;
//     struct _lnode *next;
// } ListNode;

// void solver_start_add();
// void solver_add(char c);
// ListNode *solver_solve();

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "solver.h"
#include "hashset.h"

#define VERSION "0.1"

static Trie_t dict;

typedef char BOOL;
#define YES 1
#define NO 0

typedef struct {
    char c;
    int x;
    int y;
} Piece;

int neighbors[8][2] = { {-1, -1}, {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0} };

Piece board[4][4];
int idx;

Hashset found;

BOOL dict_is_word(char *word)
{
    return (BOOL)trie_get(&dict, word);
}

BOOL dict_is_prefix(char *prfx)
{
    return trie_contains_path(&dict, prfx);
}

void solver_start_add()
{
    idx = 0;
}

void solver_add(char c)
{
    if(idx > 15)
        return;

    int x = idx % 4;
    int y = idx / 4;

    Piece p = {c, x, y};
    board[x][y] = p;

    idx++;
}

BOOL list_contains(ListNode *node, Piece *p)
{
    for(; node; node = node->next)
        if(node->value == p)
            return YES;

    return NO;
}

void list_add(ListNode **node, void *value)
{
    ListNode *top = *node;
    if(top)
    {
        for(; top->next; top = top->next);
    }

    ListNode *next = malloc(sizeof(ListNode));
    next->value = value;
    next->next = NULL;
    
    if(top)
        top->next = next;
    else
        *node = next;
}

void *list_last(ListNode *node)
{
    for(; node->next; node = node->next);
    return node->value;
}

void *pop(ListNode **node)
{
    void *value = (*node)->value;
    ListNode *next = (*node)->next;
    free(*node);
    *node = next;
    return value;
}

ListNode *duplicate_list(ListNode *start)
{
    ListNode *new = malloc(sizeof(ListNode));
    ListNode *curr = new;
    new->value = start->value;
    new->next = NULL;

    for(; start->next; start = start->next)
    {
        new->next = malloc(sizeof(ListNode));
        new->next->value = start->next->value;
        new->next->next = NULL;
        new = new->next;
    }

    return curr;
}

void list_to_str(ListNode *start, char *buf)
{
    int i;
    for(i = 0; start; i++, start = start->next)
        buf[i] = ((Piece *)start->value)->c;

    buf[i] = '\0';
}

void list_free(ListNode *node)
{
    while(node)
    {
        ListNode *next = node->next;
        free(node);
        node = next;
    }
}

void solve_from(Piece *start)
{
    ListNode *work = malloc(sizeof(ListNode));
    ListNode *first = malloc(sizeof(ListNode));
    first->value = start;
    first->next = NULL;
    work->value = first;
    work->next = NULL;

    while(work)
    {
        ListNode *current = pop(&work);

        int i;
        for(i = 0; i < 8; i++)
        {
            Piece *last = list_last(current);
            int nX = neighbors[i][0] + last->x;
            int nY = neighbors[i][1] + last->y;

            if(nX < 0 || nX > 3 || nY < 0 || nY > 3)
                continue;

            Piece *next = &board[nX][nY];
            if(list_contains(current, next))
                continue;

            ListNode *newlist = duplicate_list(current);

            list_add(&newlist, next);

            char candidate[100];
            list_to_str(newlist, candidate);

            if(dict_is_word(candidate) && strlen(candidate) > 2)
                HS_add(&found, candidate);
            if(dict_is_prefix(candidate))
                list_add(&work, newlist);
            else
                list_free(newlist);
        }

        list_free(current);
    }
}

Hashset solver_solve(Trie_t d)
{
    dict = d;
    found = HS_create(200, YES);
    int x, y;
    for(x = 0; x < 4; x++)
    {
        for(y = 0; y < 4; y++)
        {
            solve_from(&board[x][y]);
        }
    }

    return found;
}

// A B C D 
// E F G H 
// I J K L 
// M N O P 
