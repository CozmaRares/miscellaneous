#ifndef HELPER_HPP_
#define HELPER_HPP_

enum Tokens {
    NUMBER = 256,
    IDENTIFIER,
    IF,
    ELSE,
    WHILE,
    DO,
    VAR,
    ASSIGN,
};

union YYLVAL {
    int iVal;
    char cVal;
};

int charToInt(const char* const str);

#endif  // HELPER_HPP_
