%{
    #include "ast.h"
    #include "tools.h"
    #include <stdlib.h>
    #include <stdio.h>
    ast_node *programBlock; /* the top level root node of our final AST */

    extern int yylex();
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
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE TEQUAL TAND TOR TNOT
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TCOMMA TDOT
%token <token> TPLUS TMINUS TMUL TDIV TMOD TPOW
%token <token> TIF TELIF TELSE TPRT TCOMMENT TLOOP TCOLON TFUNC TSEMI

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (NIdentifier*). It makes the compiler happy.
 */
%type <node> program stmts stmt expression ifblock block elifblock elifblocks elseblock loopblock funcdecl arg arglist call calllist

/* Operator precedence for mathematical operators */
%nonassoc TPRT
%left TEQUAL
%left TOR
%left TAND
%nonassoc TCEQ TCNE TCLT TCLE TCGT TCGE
%left TMINUS TPLUS
%left TDIV TMUL TMOD
%left NEG
%right TPOW
%nonassoc TNOT
%nonassoc TIF TELIF TELSE TLOOP TFUNC
%nonassoc TLPAREN

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
loopblock : TLOOP TLPAREN expression TRPAREN block { $$ = create_loop_node(NULL, $3, NULL, $5); }
    | TLOOP TLPAREN stmt stmt expression TRPAREN block { $$ = create_loop_node($3, $4, $5, $7); }
    ;
arg : TIDENTIFIER { $$ = create_value_node(VVAR, (void *)$1); }
    ;
arglist : arg
    | arglist TCOMMA arg { ast_add_node($$, $3); }
    ;
call : expression 
    ;
calllist : call
    | calllist TCOMMA call { ast_add_node($$, $3); }
    ;
funcdecl : TFUNC TLPAREN arglist TRPAREN block { $$ = create_func_decl_node($3, $5); }
    | TFUNC TLPAREN TRPAREN block { $$ = create_func_decl_node(NULL, $4); }
stmt : expression TSEMI
    | loopblock
    | ifblock
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
    | expression TLPAREN calllist TRPAREN { $$ = create_func_call_node($1, $3); }
    | expression TLPAREN TRPAREN { $$ = create_func_call_node($1, NULL); }
    | funcdecl
    | TPRT expression { $$ = create_unary_node($2, 'p'); }
    | TNOT expression { $$ = create_unary_node($2, '!'); }
    ;

%%