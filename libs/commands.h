#pragma once

#include <iostream>
#include <map>
#include <optional>
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
    Command(CommandType type, const std::optional<int> arg = {})
        : type_(type), arg_(arg) {};

    // Operator overloading
    operator std::string() const;
    bool operator==(const Command obj) const {
        return type_ == obj.type_ && arg_ == obj.arg_;
    }
    operator CommandType() const { return type_; }
    friend std::ostream &operator<<(std::ostream &out, const Command obj);

  private:
    CommandType type_;
    std::optional<int> arg_;
};

void send_command(const Command command);
