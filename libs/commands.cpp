#include "commands.h"

#include <iostream>
#include <sstream>
#include <string>

// Operator overloading
Command::operator std::string() const {
    std::stringstream ss;
    ss << commandNames.at(type_);
    if (arg_) {
        ss << " " << *arg_;
    }
    return ss.str();
}

std::ostream &operator<<(std::ostream &out, const Command obj) {
    return out << std::string(obj);
}

void send_command(const Command command) { std::cout << command << std::endl; }
