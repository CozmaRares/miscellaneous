#include <cstdlib>
#include <iostream>

int charToInt(const char* const str) {
    char* end;
    int value = std::strtol(str, &end, 10);

    if (*end != '\0') {
        std::cerr << "Failed to convert input to double: " + std::string(str);
        std::exit(EXIT_FAILURE);
    }

    return value;
}
