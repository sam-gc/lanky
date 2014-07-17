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
%token <string> TIDENTIFIER TINTEGER TFLOAT TSTRING
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE TEQUAL TAND TOR
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TCOMMA TDOT
%token <token> TPLUS TMINUS TMUL TDIV TMOD TPOW
%token <token> TIF TELIF TELSE TPRT TCOMMENT

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (NIdentifier*). It makes the compiler happy.
 */
%type <node> program stmts stmt expression ifblock block elifblock elifblocks elseblock

/* Operator precedence for mathematical operators */
%nonassoc TPRT TCOMMENT
%left TEQUAL
%left TOR
%left TAND
%nonassoc TCEQ TCNE TCLT TCLE TCGT TCGE
%left TMINUS TPLUS
%left TDIV TMUL TMOD
%right TPOW
%nonassoc TIF TELIF TELSE

%start program

%%

program : stmts { programBlock = $1; }
    ;
stmts : stmt { $$ = create_root_node(); ast_add_node($$, $1); }
    | stmts stmt { ast_add_node($1, $2); }
    ;
block : TLBRACE stmts TRBRACE { $$ = $2; }
    ;
ifblock : TIF TLPAREN expression TRPAREN block { $$ = create_if_node($3, $5); }
    | TIF TLPAREN expression TRPAREN block elseblock { $$ = create_if_node($3, $5); ast_add_if_node($$, $6); }
    | TIF TLPAREN expression TRPAREN block elifblocks elseblock { $$ = create_if_node($3, $5); ast_add_if_node($$, $6); ast_add_if_node($$, $7); }
    | TIF TLPAREN expression TRPAREN block elifblocks { $$ = create_if_node($3, $5); ast_add_if_node($$, $6); }
    ;
elifblock : TELIF TLPAREN expression TRPAREN block { $$ = create_if_node($3, $5); }
    ;
elifblocks : elifblock
    | elifblocks elifblock { ast_add_if_node($1, $2); }
    ;
elseblock : TELSE block { $$ = create_if_node(NULL, $2); }
    ;
stmt : expression
    ;

expression : 
    TINTEGER { $$ = create_value_node(VINT, (void *)$1); }
    | TFLOAT { $$ = create_value_node(VDOUBLE, (void *)$1); }
    | TSTRING { $$ = create_value_node(VSTRING, (void *)$1); }
    | TIDENTIFIER { $$ = create_value_node(VVAR, (void *)$1); }
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
    | TIDENTIFIER TEQUAL expression { $$ = create_assignment_node($1, $3); }
    | ifblock
    | TPRT expression { $$ = create_unary_node($2, 'p'); }
    ;

%%