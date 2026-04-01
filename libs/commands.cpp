#include "commands.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

// Operator overloading
Command::operator std::string() const {
    std::stringstream ss;
    ss << commandNames.at(static_cast<size_t>(type_));
    if (arg_) {
        ss << " " << *arg_;
    }
    return ss.str();
}

std::ostream &operator<<(std::ostream &out, const Command obj) {
    return out << std::string(obj);
}

void CommandQueue::put(const Command command) {
    std::scoped_lock lock{queue_mutex};
    queue.push(command);
    queue_condition.notify_one();
}

Command CommandQueue::get() {
    std::unique_lock lock{queue_mutex};
    queue_condition.wait(lock, [this] { return !queue.empty(); });
    auto command = queue.front();
    queue.pop();
    lock.unlock();
    return command;
}

void CommandQueue::clear() {
    std::scoped_lock lock{queue_mutex};
    while (!queue.empty()) {
        queue.pop();
    }
}

void send_command(const Command command) { std::cout << command << std::endl; }

std::optional<Command> receive_command() {
    std::string input_line, command_str, argument_str;
    CommandType command_type;
    std::optional<int> arg = {};

    std::getline(std::cin, input_line);
    std::istringstream input_stream(input_line);
    if (input_stream >> command_str) {
        auto it =
            std::find(commandNames.begin(), commandNames.end(), command_str);
        if (it == commandNames.end()) {
            // Command not found
            return std::nullopt;
        }
        command_type =
            static_cast<CommandType>(std::distance(commandNames.begin(), it));
    } else {
        // No command entered
        return std::nullopt;
    }

    if (input_stream >> argument_str) {
        try {
            arg = std::stoi(argument_str);
        } catch (const std::invalid_argument &e) {
            // Could not convert argument to int
            std::cerr << "Command " << command_str
                      << " got invalid argument: " << argument_str << std::endl;
            return std::nullopt;
        }
    }

    return Command(command_type, arg);
}
