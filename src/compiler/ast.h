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

#ifndef AST_H
#define AST_H

#include "mempool.h"

//#define DEBUG(txt) printf("%d %s\n", __LINE__, txt )

extern lky_mempool ast_memory_pool;

// An enumeration of all the different possible types
// of AST nodes. The compiler will walk the tree and
// decide what to do based on this enum.
typedef enum {
    ABINARY_EXPRESSION,
    AUNARY_EXPRESSION,
    AVALUE,
    AUNIT,
    ABLOCK,
    AIF,
    AFUNC_DECL,
    ACLASS_DECL,
    AFUNC_CALL,
    ALOOP,
    ACOND_CHAIN,
    ATERNARY,
    AMEMBER_ACCESS,
    AARRAY,
    ATABLE,
    AINDEX,
    AONEOFF,
    ALOAD
} ast_type;

// The different value types...
typedef enum {
    VDOUBLE,
    VINT,
    VSTRING,
    VOBJECT,
    VVAR,
    VNONE
} ast_value_type;

// The different loop types...
typedef enum {
	LFOR,
	LWHILE
} ast_loop_type;

// A union to contain the value for
// a value node
typedef union {
    double d;
    long long i;
    char *s;
    void *o;
} ast_value_union;

// A helper wrapper to contain all
// the requisite information for
// a value
typedef struct {
    ast_value_type type;
    ast_value_union value;
} ast_value_wrapper;

// The base struct, ast_node. All
// other nodes should derive their
// structure from this one, and
// all externally visible references
// to ast_nodes should be pointed
// to by a pointer of this type,
// except where the specific fields
// are needed.
typedef struct ast_node {
    ast_type type;
    struct ast_node *next;
} ast_node;

// A binary node, which can have
// a left tree and a right tree
// with an operator in the middle
// (i.e. 3 + 4)
typedef struct ast_binary_node {
    ast_type type;
    struct ast_node *next;

    struct ast_node *left;
    struct ast_node *right;
    char opt;
} ast_binary_node;

// A unary node for things like
// _prt, !, and -neg
typedef struct {
    ast_type type;
    struct ast_node *next;

    struct ast_node *target;
    char opt;
} ast_unary_node;

// A load node, used to load
// other files and modules
typedef struct {
    ast_type type;
    struct ast_node *next;

    char *name;
} ast_load_node;

// A value node that wraps constants
// and variables
typedef struct ast_value_node {
    ast_type type;
    struct ast_node *next;

    ast_value_type value_type;
    ast_value_union value;
} ast_value_node;

// A special variation of the value
// node that wraps units
typedef struct ast_unit_value_node {
    ast_type type;
    struct ast_node *next;

    double val;
    char *fmt;
} ast_unit_value_node;

// A collection of statements/nodes;
// this struct is used for conditions,
// loops, and functions.
typedef struct ast_block_node {
    ast_type type;
    struct ast_node *next;

    struct ast_node *payload;
} ast_block_node;

// An ast_node that can represent an
// array literal
typedef struct ast_array_node {
    ast_type type;
    struct ast_node *next;

    struct ast_node *list;
} ast_array_node;

// An ast_node that can represent a
// table literal
typedef struct ast_table_node {
    ast_type type;
    struct ast_node *next;

    struct ast_node *list;
} ast_table_node;

// The index operator, i.e. arr[2]
typedef struct ast_index_node {
    ast_type type;
    struct ast_node *next;

    struct ast_node *target;
    struct ast_node *indexer;
} ast_index_node;

// A node for conditionals
typedef struct {
    ast_type type;
    struct ast_node *next;

    struct ast_node *next_if;
    struct ast_node *condition;
    struct ast_node *payload;
} ast_if_node;

typedef struct {
    ast_type type;
    struct ast_node *next;

    char opt;
    struct ast_node *left;
    struct ast_node *right;
} ast_cond_node;

// A node to represent function
// declarations
typedef struct {
    ast_type type;
    struct ast_node *next;
    
    struct ast_node *params;
    struct ast_node *payload;
} ast_func_decl_node;

// A node to represent class
// declarations
typedef struct {
    ast_type type;
    struct ast_node *next;

    // refname is the 'self' token in the example below:
    //    class() -> self { ... };
    // it is used to reference the particular instance
    // of the class. This allows the user to explicitly
    // name the 'this' variable in C++/Java and the 'self'
    // variable in Objective-C. The Python way of adding
    // an invisible argument to method names confuses me,
    // but I like the ability to change the keyword. This
    // is the compromise.
    char *refname;
    struct ast_node *payload;
} ast_class_decl_node;

// A loop node to represent loop syntax. There
// are two types of loops currently: standard
// and for loops. With standard loops (think 'while'),
// only the condition and payload fields are used,
// whereas the for loop uses all four 'init', 'condition',
// 'onloop', and 'payload' are ued.
typedef struct {
    ast_type type;
    struct ast_node *next;

    ast_loop_type loop_type;
    struct ast_node *init;
    struct ast_node *condition;
    struct ast_node *onloop;
    struct ast_node *payload;
} ast_loop_node;

// A node that represents a function call
typedef struct {
    ast_type type;
    struct ast_node *next;

    struct ast_node *ident;
    struct ast_node *arguments;
} ast_func_call_node;

// A node for the special turnary syntax
typedef struct {
    ast_type type;
    struct ast_node *next;

    struct ast_node *condition;
    struct ast_node *first;
    struct ast_node *second;
} ast_ternary_node;

// A node to access members of an object;
// typically in other languages this is
// the 'dot' operator.
typedef struct {
    ast_type type;
    struct ast_node *next;

    struct ast_node *object;
    char *ident;
} ast_member_access_node;

// A node that represents one-off
// statements (i.e. continue)
typedef struct {
    ast_type type;
    struct ast_node *next;

    char opt;
} ast_one_off_node;

void ast_init();
ast_node *create_root_node();
void ast_add_node(ast_node *curr, ast_node *next);

// All of the helper functions that Bison uses to build the
// AST with the given grammar. As mentioned earlier, it is
// important that all of these functions return the abstract
// type 'ast_node' even though under the hood they are
// creating specific instances of the structs defined above.
ast_node *create_value_node(ast_value_type type, void *data);
ast_node *create_unit_value_node(char *valstr, char *fmt);
ast_node *create_binary_node(ast_node *left, ast_node *right, char opt);
ast_node *create_unary_node(ast_node *target, char opt);
ast_node *create_assignment_node(char *left, ast_node *right);
ast_node *create_load_node(void *data);
ast_node *create_block_node(ast_node *payload);
ast_node *create_if_node(ast_node *condition, ast_node *payload);
ast_node *create_cond_node(ast_node *left, ast_node *right, char type);
ast_node *create_loop_node(ast_node *init, ast_node *condition, ast_node *onloop, ast_node *payload);
ast_node *create_func_decl_node(ast_node *params, ast_node *payload);
ast_node *create_class_decl_node(char *refname, ast_node *payload); 
ast_node *create_func_call_node(ast_node *ident, ast_node *arguments);
ast_node *create_ternary_node(ast_node *condition, ast_node *first, ast_node *second);
ast_node *create_array_node(ast_node *payload);
ast_node *create_table_node(ast_node *payload);
ast_node *create_index_node(ast_node *target, ast_node *indexer);
ast_node *create_member_access_node(ast_node *object, char *ident);
ast_node *create_one_off_node(char opt);
void ast_add_if_node(ast_node *curr, ast_node *next);

void ast_free(ast_node *node);

#endif
