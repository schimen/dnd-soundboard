#pragma once

#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>

enum class CommandType {
    PLAY_SOUND,
    STOP_SOUND,
    NEW_BANK,
    VOLUME,
    LOOP_ON,
    LOOP_OFF,
    EXIT,
};

const std::map<CommandType, std::string_view> commandNames = {
    {CommandType::PLAY_SOUND, "play"}, {CommandType::STOP_SOUND, "stop"},
    {CommandType::NEW_BANK, "bank"},   {CommandType::VOLUME, "volume"},
    {CommandType::LOOP_ON, "loop"},    {CommandType::LOOP_OFF, "oneshot"},
    {CommandType::EXIT, "exit"}};

class Command {
  public:
    Command(CommandType type_, const std::optional<int> arg = {})
        : type(type_), arg(arg) {};

    // Operator overloading
    operator std::string() const;
    friend std::ostream &operator<<(std::ostream &out, const Command obj);

  private:
    CommandType type;
    std::optional<int> arg;
};

template <> struct std::formatter<Command> : std::formatter<std::string> {
    auto format(const Command &obj, std::format_context &ctx) const {
        return std::format_to(ctx.out(), "{}", std::string(obj));
    }
};

void send_command(const Command command);
