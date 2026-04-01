#pragma once

#include <array>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>

enum class CommandType {
    PLAY_SOUND,
    STOP_SOUND,
    NEW_BANK,
    VOLUME,
    LOOP_ON,
    LOOP_OFF,
    EXIT,
};

constexpr std::array<std::string_view, 7> commandNames = {
    "play", "stop", "bank", "volume", "loop", "oneshot", "exit"};

class Command {
  public:
    Command(CommandType type, const std::optional<int> arg = {})
        : type_(type), arg_(arg) {};
    std::optional<int> getArg() const { return arg_; }
    CommandType getType() const { return type_; }

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

class CommandQueue {
  public:
    void put(const Command command);
    Command get();
    void clear();

  private:
    std::queue<Command> queue;
    std::condition_variable queue_condition;
    std::mutex queue_mutex;
};

void send_command(const Command command);

std::optional<Command> receive_command();
