all:
	lex -o lexer.cpp --header-file=lexer.hpp lexer.l
	clang++ -g main.cpp lexer.cpp helper.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -o main

check:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./main

clean:
	rm lexer.cpp main
