#pragma once

#include <concepts>
#include <map>
#include <string_view>

enum class KeyStateEnum { RELEASED, PRESSED, HELD };

const std::map<KeyStateEnum, std::string_view> stateNames = {
    {KeyStateEnum::RELEASED, "released"},
    {KeyStateEnum::PRESSED, "pressed"},
    {KeyStateEnum::HELD, "held"}};

class KeyState {
  public:
    KeyState() = default;
    KeyState(std::integral auto value) {
        switch (value) {
        case 1:
            state = KeyStateEnum::PRESSED;
            break;
        case 2:
            state = KeyStateEnum::HELD;
            break;
        default:
            state = KeyStateEnum::RELEASED;
            break;
        }
    }

    // Functions to check the state of a key event
    bool isReleased() { return state == KeyStateEnum::RELEASED; };
    bool isPressed() { return state == KeyStateEnum::PRESSED; };
    bool isHeld() { return state == KeyStateEnum::HELD; };
    bool isActive() { return isPressed() || isHeld(); };

    // Operator overloading
    bool operator==(const KeyState obj) const { return state == obj.state; }
    template <std::integral T> operator T() const {
        return static_cast<T>(state);
    }
    operator std::string_view() const { return stateNames.at(state); }
    friend std::ostream &operator<<(std::ostream &out, const KeyState obj) {
        return out << std::string_view(obj);
    }

  private:
    KeyStateEnum state = KeyStateEnum::RELEASED;
};
