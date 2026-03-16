#pragma once

#include <array>
#include <concepts>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

class KeyState {
  public:
    KeyState() = default;
    KeyState(std::integral auto value) {
        switch (value) {
        case 1:
            state = KEY_PRESSED;
            break;
        case 2:
            state = KEY_HELD;
            break;
        default:
            state = KEY_RELEASED;
            break;
        }
    };

    // Functions to check the state of a key event
    bool isReleased() { return state == KEY_RELEASED; };
    bool isPressed() { return state == KEY_PRESSED; };
    bool isHeld() { return state == KEY_HELD; };
    bool isActive() { return isPressed() || isHeld(); };

    // Operator overloading
    bool operator==(const KeyState obj) const { return state == obj.state; }
    template <std::integral T> operator T() const {
        return static_cast<T>(state);
    }
    friend std::ostream &operator<<(std::ostream &out, const KeyState obj) {
        return out << stateNames[obj.state];
    }
    operator std::string() const { return std::string(stateNames[state]); }

  private:
    enum key_state_enum {
        KEY_RELEASED,
        KEY_PRESSED,
        KEY_HELD
    } state = KEY_RELEASED;
    static constexpr std::array<std::string_view, 3> stateNames = {
        "released", "pressed", "held"};
};

template <> struct std::formatter<KeyState> : std::formatter<std::string> {
    auto format(const KeyState &obj, std::format_context &ctx) const {
        return std::format_to(ctx.out(), "{}", std::string(obj));
    }
};
