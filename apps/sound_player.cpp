#include <iostream>
#include <string>

int main() {
    std::string input_line;
    std::cout << "Sound player" << std::endl;
    while (std::cin) {
        getline(std::cin, input_line);
        std::cout << input_line << std::endl;
    };
    return 0;
}
