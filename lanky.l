%{
#include "ast.h"
#include "tools.h"
#include "parser.h"
#define SAVE_TOKEN (yylval.string = alloc_str(yytext))
#define TOKEN(t) (yylval.token = t)

%}

%option noyywrap

white [ \t\n]+
digit [0-9]
lalpha [a-z]
alpha [a-zA-Z_]
integer {digit}+
exponent [eE][+-]?{integer}
real {integer}("."{integer}){exponent}?
nvar {alpha}({alpha}|{digit})*

%%

{white}     ;
{nvar}      {SAVE_TOKEN; return TIDENTIFIER;}
{real}      {SAVE_TOKEN; return TFLOAT;}
{integer}   {SAVE_TOKEN; return TINTEGER;}
"="                     return TOKEN(TEQUAL);
"=="                    return TOKEN(TCEQ);
"!="                    return TOKEN(TCNE);
"<"                     return TOKEN(TCLT);
"<="                    return TOKEN(TCLE);
">"                     return TOKEN(TCGT);
">="                    return TOKEN(TCGE);
"("                     return TOKEN(TLPAREN);
")"                     return TOKEN(TRPAREN);
"{"                     return TOKEN(TLBRACE);
"}"                     return TOKEN(TRBRACE);
"."                     return TOKEN(TDOT);
","                     return TOKEN(TCOMMA);
"+"                     return TOKEN(TPLUS);
"-"                     return TOKEN(TMINUS);
"*"                     return TOKEN(TMUL);
"/"                     return TOKEN(TDIV);
"%"                     return TOKEN(TMOD);
"^"                     return TOKEN(TPOW);
"&"                     return TOKEN(TAND);
"|"                     return TOKEN(TOR);
.                       printf("Unknown token! %s\n", yytext); yyterminate();
%%
