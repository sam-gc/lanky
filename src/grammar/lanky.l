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
#include "parser.h"
#define SAVE_TOKEN (yylval.string = alloc_str(yytext))
#define TOKEN(t) (yylval.token = t)
char yyyhad_error = 0;
int yyerror(char *s)
    {
          printf("%d: %s at %s\n", yylineno, s, yytext);
          yyyhad_error = 1;
    }

int fileno(FILE *stream);
%}

%option noyywrap
%option yylineno

white [ \t]+
digit [0-9]
lalpha [a-z]
alpha [a-zA-Z_$]
integer {digit}+
exponent [eE][+-]?{integer}
real {integer}("."{digit}*){exponent}?
nvar {alpha}({alpha}|{digit})*
strlit (\"(\\.|[^"])*\"|'(\\.|[^'])*')
comment (\#|(\-\-)).*(\n)

%%

{white}     ;
\n {}
"if"					return TOKEN(TIF);
{comment}     ;
"elif"                  return TOKEN(TELIF);
"else"                  return TOKEN(TELSE);
"_prt"                  return TOKEN(TPRT);
"for"                  return TOKEN(TLOOP);
"continue"              return TOKEN(TCONTINUE);
"break"                 return TOKEN(TBREAK);
"func"                  return TOKEN(TFUNC);
"ret"                   return TOKEN(TRET);
"raise"                 return TOKEN(TRAISE);
"->"                   return TOKEN(TARROW);
"class"                 return TOKEN(TCLASS);
"=>"                    return TOKEN(TRARROW);
"nil"                   return TOKEN(TNIL);
"load"                  return TOKEN(TLOAD);
"in"                    return TOKEN(TIN);
"try"                   return TOKEN(TTRY);
"catch"                 return TOKEN(TCATCH);
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
"+="                    return TOKEN(TPLUSE);
"-"                     return TOKEN(TMINUS);
"-="                    return TOKEN(TMINUSE);
"["                     return TOKEN(TLBRACKET);
"]"                     return TOKEN(TRBRACKET);
"*"                     return TOKEN(TMUL);
"*="                    return TOKEN(TMULE);
"/"                     return TOKEN(TDIV);
"/="                    return TOKEN(TDIVE);
"%"                     return TOKEN(TMOD);
"%="                    return TOKEN(TMODE);
"^"                     return TOKEN(TPOW);
"^="                    return TOKEN(TPOWE);
"&"                     return TOKEN(TAND);
"&="                    return TOKEN(TANDE);
"|"                     return TOKEN(TOR);
"|="                    return TOKEN(TORE);
":"                     return TOKEN(TCOLON);
"!"                     return TOKEN(TNOT);
"?"                     return TOKEN(TQUESTION);
"?:"                    return TOKEN(TNILOR);
"??"                    return TOKEN(TCON);
"??="                   return TOKEN(TCONE);
"\\&"                    return TOKEN(TBAND);
"\\&="                   return TOKEN(TBANDE);
"\\|"                    return TOKEN(TBOR);
"\\|="                   return TOKEN(TBORE);
"\\^"                    return TOKEN(TBXOR);
"\\^="                   return TOKEN(TBXORE);
"\\<"                    return TOKEN(TBLSHIFT);
"\\>"                    return TOKEN(TBRSHIFT);
"\\<="                   return TOKEN(TBLSHIFTE);
"\\>="                   return TOKEN(TBRSHIFTE);
";"                     return TOKEN(TSEMI);
.                       printf("Unknown token! %s\n", yytext); yyterminate();
%%
