%{
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

#include "helper.h"

void yyerror(const char *msg);
%}

%define parse.error verbose
%locations

%start file

%union {
     int int_val;
     struct monome_t* mono_val;
     struct polynome_t* poly_val;
}

%token <int_val> NUMBER
%token VALUE

%type <poly_val> expr
%type <mono_val> mono

%left '-' '+'

%%

file : file statement '\n'
     | file '$' '\n' { printf("\n"); }
     | file '\n'
     | /* empty */
     ;

statement : expr { print_poly($1); }
          | VALUE '[' expr ',' NUMBER ']' { int e = eval_poly($3, $5); printf("%d\n", e); }
          ;

expr : expr '+' expr { $$ = add_polys($1, $3); }
     | expr '-' expr { $$ = sub_polys($1, $3); }
     | expr '*' expr { $$ = multiply_polys($1, $3); }
     | '(' expr ')' '\'' { $$ = dx_poly($2); }
     | '(' expr ')' { $$ = $2; }
     | mono { Polynome* p = make_poly(); add_to_poly(p, $1); $$ = p; }
     ;

mono : NUMBER '*' 'Y' '^' NUMBER { $$ = make_mono($1, $5); }
       | 'Y' '^' NUMBER { $$ = make_mono(1, $3); }
       | NUMBER '*' 'Y' { $$ = make_mono($1, 1); }
       | 'Y' { $$ = make_mono(1, 1); }
       | NUMBER { $$ = make_mono($1, 0); }
       ;

%%

int main(int argc, char **argv) {
   if (argc > 1) {
      yyin = fopen(argv[1], "r");
      if (yyin == NULL){
         printf("syntax: %s filename\n", argv[0]);
      }
   }
   yyparse();
   return 0;
}

void yyerror(const char *msg) {
   printf("** Line %d: %s\n", yylloc.first_line, msg);
}
