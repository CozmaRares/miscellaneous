%{
#include "helper.hpp"
extern YYLVAL yylval;
%}

%option noyywrap

%%

-?[0-9]+ { yylval.iVal = charToInt(yytext); return NUMBER; }

[a-zA-Z] { yylval.cVal = yytext[0]; return IDENTIFIER; }

[+\-\*/()=\n{};] { return *yytext; }

if { return IF; }
else { return ELSE; }

while { return WHILE; }
do { return DO; }

var { return VAR; }
assign { return ASSIGN; }

[ \t]+ ;
