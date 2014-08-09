%{
#include "ast.h"
#include "tools.h"
#include "parser.h"
#define SAVE_TOKEN (yylval.string = alloc_str(yytext))
#define TOKEN(t) (yylval.token = t)
void yyerror(char *s)
    {
          printf("%d: %s at %s\n", yylineno, s, yytext);
    }
%}

%option noyywrap
%option yylineno

white [ \t]+
digit [0-9]
lalpha [a-z]
alpha [a-zA-Z_]
integer {digit}+
exponent [eE][+-]?{integer}
real {integer}("."{digit}*){exponent}?
nvar {alpha}({alpha}|{digit})*
strlit \"(\\.|[^"])*\"
comment (\#|(\-\-)).*(\n)

%%

{white}     ;
\n {}
"if"					return TOKEN(TIF);
{comment}     ;
"elif"                  return TOKEN(TELIF);
"else"                  return TOKEN(TELSE);
"_prt"                  return TOKEN(TPRT);
"loop"                  return TOKEN(TLOOP);
"func"                  return TOKEN(TFUNC);
"ret"                   return TOKEN(TRET);
"->"                   return TOKEN(TARROW);
"class"                 return TOKEN(TCLASS);
{nvar}      {SAVE_TOKEN; return TIDENTIFIER;}
{real}      {SAVE_TOKEN; return TFLOAT;}
{integer}   {SAVE_TOKEN; return TINTEGER;}
{strlit}	{SAVE_TOKEN; return TSTRING;}
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
":"                     return TOKEN(TCOLON);
"!"                     return TOKEN(TNOT);
"?"                     return TOKEN(TQUESTION);
";"                     return TOKEN(TSEMI);
.                       printf("Unknown token! %s\n", yytext); yyterminate();
%%
