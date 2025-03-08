%{
#include "parser.h"

extern YYSTYPE yylval;
%}

%option noyywrap

%%

[\n$Y\^\+\-\(\)\*'\[\],] { return *yytext; }

[0-9]+ {
	yylval.int_val = atoi(yytext);
	return NUMBER;
}

value {
	return VALUE;
}

[ \t]+          ;
