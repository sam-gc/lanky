%{
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

    #include "ast.h"
    #include "tools.h"
    #include "class_builder.h"
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
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE TEQUAL TAND TOR TNOT TBAND TBOR TBXOR TBLSHIFT TBRSHIFT 
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TLBRACKET TRBRACKET TCOMMA TDOT
%token <token> TPLUS TMINUS TMUL TDIV TMOD TPOW TCON TIN TRARROW
%token <token> TPLUSE TMINUSE TMULE TDIVE TMODE TPOWE TORE TANDE TCONE TBANDE TBORE TBXORE TBLSHIFTE TBRSHIFTE
%token <token> TIF TELIF TELSE TPRT TCOMMENT TLOOP TCOLON TFUNC TSEMI TRET TQUESTION TARROW TCLASS TNIL TCONTINUE TBREAK TLOAD TNILOR TRAISE
%token <token> TTRY TCATCH
%token <token> TINIT TPROTO TSTATIC

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (NIdentifier*). It makes the compiler happy.
 */
%type <node> program stmts stmt expression ifblock block elifblock elifblocks elseblock loopblock funcdecl arg arglist call calllist memaccess classdecl arrdecl arraccess opapply tabset tabsetlist tabdecl binor binand objset objsetlist objdecl trycatchblock classmember classmemberlist

/* Operator precedence for mathematical operators */
%nonassoc TPRT TRET TNIL TRAISE
%left TEQUAL
%left TRARROW
%left TPLUSE TMINUSE TMULE TDIVE TMODE TPOWE TORE TANDE TBANDE TBORE TBXORE TBLSHIFTE TBRSHIFTE
%left TQUESTION TCOLON TNILOR TCON TCONE
%right TOR
%right TAND
%nonassoc TCEQ TCNE TCLT TCLE TCGT TCGE
%left TBOR
%left TBXOR
%left TBAND
%left TBLSHIFT TBRSHIFT
%left TMINUS TPLUS
%left TDIV TMUL TMOD
%left NEG
%right TPOW
%nonassoc TNOT
%nonassoc TIF TELIF TELSE TLOOP TFUNC TCLASS TTRY TCATCH
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
    | TLOOP TIDENTIFIER TIN expression block { $$ = create_iter_loop_node(create_value_node(VVAR, (void *)$2), NULL, $4, $5); }
    | TLOOP TIDENTIFIER TCOMMA TIDENTIFIER TIN expression block {
        $$ = create_iter_loop_node(create_value_node(VVAR, (void *)$2), create_value_node(VVAR, (void *)$4), $6, $7); }
    ;
arg : TIDENTIFIER { $$ = create_value_node(VVAR, (void *)$1); }
    ;
trycatchblock: TTRY block TCATCH TIDENTIFIER block { $$ = create_try_catch_node($2, $5, create_value_node(VVAR, (void *)$4)); }
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
funcdecl : TFUNC TLPAREN arglist TRPAREN block { $$ = create_func_decl_node($3, $5, NULL); }
    | TFUNC TLPAREN TRPAREN block { $$ = create_func_decl_node(NULL, $4, NULL); }
    | TFUNC TLPAREN arglist TRPAREN TARROW TIDENTIFIER block { $$ = create_func_decl_node($3, $7, $6); }
    | TFUNC TLPAREN TRPAREN TARROW TIDENTIFIER block { $$ = create_func_decl_node(NULL, $6, $5); }
    ;
/*classdecl : TCLASS TLPAREN TRPAREN TARROW TIDENTIFIER block { $$ = create_class_decl_node($5, $6); }
    ;*/
classmemberlist : classmember
    | classmemberlist classmember { ast_add_node($$, $2); }
    ;
classmember : TPROTO TIDENTIFIER TCOLON expression { $$ = create_class_member_node(LCP_PROTO, $2, $4); }
    | TSTATIC TIDENTIFIER TCOLON expression { $$ = create_class_member_node(LCP_STATIC, $2, $4); }
    | TINIT TIDENTIFIER TCOLON funcdecl { $$ = create_class_member_node(LCP_INIT, $2, $4); }
    ;
classdecl : TCLASS TLBRACE classmemberlist TRBRACE { $$ = create_class_decl_node($3, NULL); }
    | TCLASS TLPAREN expression TRPAREN TLBRACE classmemberlist TRBRACE { $$ = create_class_decl_node($6, $3); }
    ;
stmt : expression TSEMI
    | TRET TSEMI { $$ = create_unary_node(NULL, 'r'); } 
    | loopblock
    | ifblock
    | trycatchblock
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

objset : TDOT TIDENTIFIER TCOLON expression { $$ = create_value_node(VVAR, (void *)$2); ast_add_node($$, $4); }
    ;
objsetlist : objset
    | objsetlist TCOMMA objset { ast_add_node($1, $3); }
    ;
objdecl : TLBRACE TDOT TRBRACE { $$ = create_unary_node(NULL, '1'); }
    | TLBRACE objsetlist TRBRACE { $$ = create_object_decl_node($2, NULL, NULL); }
    | TLBRACE objsetlist TRBRACE TARROW TIDENTIFIER { $$ = create_object_decl_node($2, $5, NULL); }
    | TIDENTIFIER TARROW TLBRACE objsetlist TRBRACE { $$ = create_object_decl_node($4, $1, NULL); }
    | expression TRARROW TLBRACE objsetlist TRBRACE { $$ = create_object_decl_node($4, NULL, $1);  }
    | expression TRARROW TIDENTIFIER TARROW TLBRACE objsetlist TRBRACE { $$ = create_object_decl_node($6, $3, $1); }
    ;

opapply : TIDENTIFIER TPLUSE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '+')); }
    | TIDENTIFIER TMINUSE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '-'))    ; }
    | TIDENTIFIER TMULE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '*'))    ; }
    | TIDENTIFIER TDIVE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '/'))    ; }
    | TIDENTIFIER TMODE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '%'))    ; }
    | TIDENTIFIER TPOWE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '^'))    ; }
    | TIDENTIFIER TORE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '|'))    ; }
    | TIDENTIFIER TANDE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '&'))    ; }
    | TIDENTIFIER TCONE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '?'))  ;}
    | TIDENTIFIER TBANDE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, 'a'))  ;}
    | TIDENTIFIER TBORE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, 'o'))  ;}
    | TIDENTIFIER TBXORE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, 'x'))  ;}
    | TIDENTIFIER TBLSHIFTE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '<'))  ;}
    | TIDENTIFIER TBRSHIFTE expression { $$ = create_assignment_node($1, create_binary_node(create_value_node(VVAR, (void *)$1), $3, '>'))  ;}
    | arraccess TPLUSE expression { $$ = create_triple_set_node($1, $3, '+'); }
    | arraccess TMINUSE expression { $$ = create_triple_set_node($1, $3, '-'); }
    | arraccess TMULE expression { $$ = create_triple_set_node($1, $3, '*'); }
    | arraccess TDIVE expression { $$ = create_triple_set_node($1, $3, '/'); }
    | arraccess TMODE expression { $$ = create_triple_set_node($1, $3, '%'); }
    | arraccess TPOWE expression { $$ = create_triple_set_node($1, $3, '^'); }
    | arraccess TORE expression { $$ = create_triple_set_node($1, $3, '|'); }
    | arraccess TANDE expression { $$ = create_triple_set_node($1, $3, '&'); }
    | arraccess TCONE expression { $$ = create_triple_set_node($1, $3, '?'); }
    | arraccess TBANDE expression { $$ = create_triple_set_node($1, $3, 'a'); }
    | arraccess TBORE expression { $$ = create_triple_set_node($1, $3, 'o'); }
    | arraccess TBXORE expression { $$ = create_triple_set_node($1, $3, 'x'); }
    | arraccess TBLSHIFTE expression { $$ = create_triple_set_node($1, $3, '<'); }
    | arraccess TBRSHIFTE expression { $$ = create_triple_set_node($1, $3, '>'); }
    | memaccess TPLUSE expression { $$ = create_triple_set_node($1, $3, '+'); }
    | memaccess TMINUSE expression { $$ = create_triple_set_node($1, $3, '-'); }
    | memaccess TMULE expression { $$ = create_triple_set_node($1, $3, '*'); }
    | memaccess TDIVE expression { $$ = create_triple_set_node($1, $3, '/'); }
    | memaccess TMODE expression { $$ = create_triple_set_node($1, $3, '%'); }
    | memaccess TPOWE expression { $$ = create_triple_set_node($1, $3, '^'); }
    | memaccess TORE expression { $$ = create_triple_set_node($1, $3, '|'); }
    | memaccess TANDE expression { $$ = create_triple_set_node($1, $3, '&'); }
    | memaccess TCONE expression { $$ = create_triple_set_node($1, $3, '?'); }
    | memaccess TBANDE expression { $$ = create_triple_set_node($1, $3, 'a'); }
    | memaccess TBORE expression { $$ = create_triple_set_node($1, $3, 'o'); }
    | memaccess TBXORE expression { $$ = create_triple_set_node($1, $3, 'x'); }
    | memaccess TBLSHIFTE expression { $$ = create_triple_set_node($1, $3, '<'); }
    | memaccess TBRSHIFTE expression { $$ = create_triple_set_node($1, $3, '>'); }
    ;

binor : expression TOR expression { $$ = create_cond_node($1, $3, '|'); }
    | binor TOR expression { $$ = create_cond_node($1, $3, '|'); }
    ;

binand : expression TAND expression { $$ = create_cond_node($1, $3, '&'); }
    | binand TAND expression { $$ = create_cond_node($1, $3, '&'); }
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
    | binor
    | binand
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
    | expression TCON expression { $$ = create_binary_node($1, $3, '?'); }
    | expression TBAND expression { $$ = create_binary_node($1, $3, 'a'); }
    | expression TBOR expression { $$ = create_binary_node($1, $3, 'o'); }
    | expression TBXOR expression { $$ = create_binary_node($1, $3, 'x'); }
    | expression TBLSHIFT expression { $$ = create_binary_node($1, $3, '<'); }
    | expression TBRSHIFT expression { $$ = create_binary_node($1, $3, '>'); }
    | TLPAREN expression TRPAREN { $$ = $2; }
    | TCLT TSTRING TCGT { $$ = create_load_node((void *)$2); }
    | TLOAD TSTRING { $$ = create_load_node((void *)$2); }
    | expression TQUESTION expression TCOLON expression { $$ = create_ternary_node($1, $3, $5); }
    | expression TNILOR expression { $$ = create_ternary_node($1, $1, $3); }
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
    | objdecl
    | TNIL { $$ = create_unary_node(NULL, '0'); }
    | TPRT expression { $$ = create_unary_node($2, 'p'); }
    | TNOT expression { $$ = create_unary_node($2, '!'); }
    | TRET expression { $$ = create_unary_node($2, 'r'); }
    | TRAISE expression { $$ = create_unary_node($2, 't'); }
    | TMINUS expression %prec TNEGATIVE { $$ = create_unary_node($2, '-'); }
    ;

%%
