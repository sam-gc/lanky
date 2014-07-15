%{
    #include "ast.h"
    #include "tools.h"
    #include <stdlib.h>
    #include <stdio.h>
    ast_node *programBlock; /* the top level root node of our final AST */

    extern int yylex();
    void yyerror(const char *s) { printf("ERROR: %s\n", s); }
%}

/* Represents the many different ways we can access our data */
%union {
    int token;
    char *string;
    ast_node *node;
}

/* Define our terminal symbols (tokens). This should
   match our tokens.l lex file. We also define the node type
   they represent.
 */
%token <string> TIDENTIFIER TINTEGER TFLOAT
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE TEQUAL TAND TOR
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TCOMMA TDOT
%token <token> TPLUS TMINUS TMUL TDIV TMOD TPOW

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (NIdentifier*). It makes the compiler happy.
 */
%type <node> program stmts stmt expression

/* Operator precedence for mathematical operators */
%left TOR
%left TAND
%nonassoc TCEQ TCNE TCLT TCLE TCGT TCGE TEQUAL
%left TMINUS TPLUS
%left TDIV TMUL TMOD
%right TPOW

%start program

%%

program : stmts { programBlock = $1; }
    ;
stmts : stmt { $$ = create_root_node(); ast_add_node($$, $1); }
    | stmts stmt { ast_add_node($1, $2); }
    ;
stmt : expression;

expression : 
    TINTEGER { $$ = create_value_node(VINT, (void *)$1); }
    | TFLOAT { $$ = create_value_node(VDOUBLE, (void *)$1); }
    | expression TPLUS expression { $$ = create_binary_node($1, $3, '+'); }
    | expression TMINUS expression { $$ = create_binary_node($1, $3, '-'); }
    | expression TMUL expression { $$ = create_binary_node($1, $3, '*'); }
    | expression TDIV expression { $$ = create_binary_node($1, $3, '/'); }
    | expression TMOD expression { $$ = create_binary_node($1, $3, '%'); }
    | expression TPOW expression { $$ = create_binary_node($1, $3, '^'); }
    | expression TCEQ expression { $$ = create_binary_node($1, $3, 'E'); }
    | expression TCNE expression { $$ = create_binary_node($1, $3, 'n'); }
    | expression TCLT expression { $$ = create_binary_node($1, $3, 'l'); }
    | expression TCLE expression { $$ = create_binary_node($1, $3, 'L'); }
    | expression TCGT expression { $$ = create_binary_node($1, $3, 'g'); }
    | expression TCGE expression { $$ = create_binary_node($1, $3, 'G'); }
    | expression TOR expression { $$ = create_binary_node($1, $3, '|'); }
    | expression TAND expression { $$ = create_binary_node($1, $3, '&'); }
    | TLPAREN expression TRPAREN { $$ = $2; }
    ;

%%