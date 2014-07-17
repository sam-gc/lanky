#ifndef AST_H
#define AST_H

#define DEBUG(txt) printf("%d %s\n", __LINE__, txt )

typedef enum {
	ABINARY_EXPRESSION,
	AUNARY_EXPRESSION,
	AVALUE,
	ABLOCK,
	AIF
} ast_type;

typedef enum {
	VDOUBLE,
	VINT,
	VSTRING,
	VOBJECT,
	VVAR,
	VNONE
} ast_value_type;

typedef union {
	double d;
	long long i;
	char *s;
	void *o;
} ast_value_union;

typedef struct {
	ast_value_type type;
	ast_value_union value;
} ast_value_wrapper;

typedef struct ast_node {
	ast_type type;
	struct ast_node *next;
} ast_node;

typedef struct ast_binary_node {
	ast_type type;
	struct ast_node *next;

	struct ast_node *left;
	struct ast_node *right;
	char opt;
} ast_binary_node;

typedef struct {
	ast_type type;
	struct ast_node *next;

	struct ast_node *target;
	char opt;
} ast_unary_node;

typedef struct ast_value_node {
	ast_type type;
	struct ast_node *next;

	ast_value_type value_type;
	ast_value_union value;
} ast_value_node;

typedef struct ast_block_node {
	ast_type type;
	struct ast_node *next;

	struct ast_node *payload;
} ast_block_node;

typedef struct {
	ast_type type;
	struct ast_node *next;

	struct ast_node *condition;
	struct ast_node *payload;
} ast_if_node;

ast_node *create_root_node();
void ast_add_node(ast_node *curr, ast_node *next);
ast_node *create_value_node(ast_value_type type, void *data);
ast_node *create_binary_node(ast_node *left, ast_node *right, char opt);
ast_node *create_unary_node(ast_node *target, char opt);
ast_node *create_assignment_node(char *left, ast_node *right);
ast_node *create_block_node(ast_node *payload);
ast_node *create_if_node(ast_node *condition, ast_node *payload);
void ast_free(ast_node *node);

#endif