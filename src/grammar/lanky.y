%{
    #include "ast.h"
    #include "tools.h"
    #include <stdlib.h>
    #include <stdio.h>
    ast_node *programBlock; /* the top level root node of our final AST */

    extern int yylex();
    extern int yyerror(char *);
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
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TLBRACKET TRBRACKET TCOMMA TDOT
%token <token> TPLUS TMINUS TMUL TDIV TMOD TPOW
%token <token> TPLUSE TMINUSE TMULE TDIVE TMODE TPOWE TORE TANDE
%token <token> TIF TELIF TELSE TPRT TCOMMENT TLOOP TCOLON TFUNC TSEMI TRET TQUESTION TARROW TCLASS TNIL TCONTINUE TBREAK

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (NIdentifier*). It makes the compiler happy.
 */
%type <node> program stmts stmt expression ifblock block elifblock elifblocks elseblock loopblock funcdecl arg arglist call calllist memaccess classdecl arrdecl arraccess opapply tabset tabsetlist tabdecl

/* Operator precedence for mathematical operators */
%nonassoc TPRT TRET TNIL
%left TEQUAL
%left TPLUSE TMINUSE TMULE TDIVE TMODE TPOWE TORE TANDE
%left TQUESTION TCOLON
%left TOR
%left TAND
%nonassoc TCEQ TCNE TCLT TCLE TCGT TCGE
%left TMINUS TPLUS
%left TDIV TMUL TMOD
%left NEG
%right TPOW
%nonassoc TNOT
%nonassoc TIF TELIF TELSE TLOOP TFUNC TCLASS
%nonassoc TLPAREN TLBRACKET TRBRACKET
%left TDOT
%nonassoc TNEGATIVE

%start program

%%

program : stmts { programBlock = $1; }
    ;
stmts : stmt { $$ = create_root_node(); ast_add_node($$, $1); }
    | stmts stmt { ast_add_node($1, $2); }
    ;
block : TLBRACE stmts TRBRACE { $$ = $2; }
    | TLBRACE TRBRACE { $$ = create_root_node(); }
    ;
ifblock : TIF  expression  block { $$ = create_if_node($2, $3); }
    | TIF expression  block elseblock { $$ = create_if_node($2, $3); ast_add_if_node($$, $4); }
    | TIF expression  block elifblocks elseblock { $$ = create_if_node($2, $3); ast_add_if_node($$, $4); ast_add_if_node($$, $5); }
    | TIF  expression  block elifblocks { $$ = create_if_node($2, $3); ast_add_if_node($$, $4); }
    ;
elifblock : TELIF  expression  block { $$ = create_if_node($2, $3); }
    ;
elifblocks : elifblock
    | elifblocks elifblock { ast_add_if_node($1, $2); }
    ;
elseblock : TELSE block { $$ = create_if_node(NULL, $2); }
    ;
loopblock : TLOOP  expression  block { $$ = create_loop_node(NULL, $2, NULL, $3); }
    | TLOOP  stmt stmt expression  block { $$ = create_loop_node($2, $3, $4, $5); }
    ;
arg : TIDENTIFIER { $$ = create_value_node(VVAR, (void *)$1); }
    ;
arglist : arg
    | arglist TCOMMA arg { ast_add_node($$, $3); }
    ;
tabset : expression TCOLON expression { ast_add_node($1, $3); }
    ;
tabsetlist : tabset
    | tabsetlist TCOMMA tabset { ast_add_node($$, $3); }
call : expression 
    ;
calllist : call
    | calllist TCOMMA call { ast_add_node($$, $3); }
    ;
funcdecl : TFUNC TLPAREN arglist TRPAREN block { $$ = create_func_decl_node($3, $5); }
    | TFUNC TLPAREN TRPAREN block { $$ = create_func_decl_node(NULL, $4); }
    ;
classdecl : TCLASS TLPAREN TRPAREN TARROW TIDENTIFIER block { $$ = create_class_decl_node($5, $6); }
    ;
stmt : expression TSEMI
    | loopblock
    | ifblock
    ;
memaccess : expression TDOT TIDENTIFIER { $$ = create_member_access_node($1, $3); }
    ;
arraccess : expression TLBRACKET expression TRBRACKET { $$ = create_index_node($1, $3); }
    ;
arrdecl : TLBRACKET TRBRACKET { $$ = create_array_node(NULL); }
    | TLBRACKET calllist TRBRACKET { $$ = create_array_node($2); }
    ;
tabdecl : TLBRACKET TCOLON TRBRACKET { $$ = create_table_node(NULL); }
    | TLBRACKET tabsetlist TRBRACKET { $$ = create_table_node($2); }
    ;

opapply : TIDENTIFIER TPLUSE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '+')); }
    | TIDENTIFIER TMINUSE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '-'))    ; }
    | TIDENTIFIER TMULE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '*'))    ; }
    | TIDENTIFIER TDIVE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '/'))    ; }
    | TIDENTIFIER TMODE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '%'))    ; }
    | TIDENTIFIER TPOWE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '^'))    ; }
    | TIDENTIFIER TORE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '|'))    ; }
    | TIDENTIFIER TANDE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '&'))    ; }
    ;

expression : 
    TINTEGER { $$ = create_value_node(VINT, (void *)$1); }
    | TFLOAT { $$ = create_value_node(VDOUBLE, (void *)$1); }
    | TINTEGER TSTRING { $$ = create_unit_value_node($1, $2); }
    | TFLOAT TSTRING { $$ = create_unit_value_node($1, $2); }
    | TSTRING { $$ = create_value_node(VSTRING, (void *)$1); }
    | TIDENTIFIER { $$ = create_value_node(VVAR, (void *)$1); }
    | TBREAK { $$ = create_one_off_node('b'); }
    | TCONTINUE { $$ = create_one_off_node('c'); }
    | opapply
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
    | expression TQUESTION expression TCOLON expression { $$ = create_ternary_node($1, $3, $5); }
    | TIDENTIFIER TEQUAL expression { $$ = create_assignment_node($1, $3); }
    | memaccess TEQUAL expression { $$ = create_binary_node($1, $3, '='); }
    | arraccess TEQUAL expression { $$ = create_binary_node($1, $3, '='); }
    | expression TLPAREN calllist TRPAREN { $$ = create_func_call_node($1, $3); }
    | expression TLPAREN TRPAREN { $$ = create_func_call_node($1, NULL); }
    | arraccess
    | funcdecl
    | memaccess
    | classdecl
    | arrdecl
    | tabdecl
    | TNIL { $$ = create_unary_node(NULL, '0'); }
    | TPRT expression { $$ = create_unary_node($2, 'p'); }
    | TNOT expression { $$ = create_unary_node($2, '!'); }
    | TRET expression { $$ = create_unary_node($2, 'r'); }
    | TMINUS expression %prec TNEGATIVE { $$ = create_unary_node($2, '-'); }
    ;

%%
