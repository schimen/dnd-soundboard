#include "keystate.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <sstream>

TEST_CASE("KeyState default constructor creates released state") {
    KeyState defaultKeyState;
    REQUIRE(defaultKeyState.isReleased());
    REQUIRE_FALSE(defaultKeyState.isPressed());
    REQUIRE_FALSE(defaultKeyState.isHeld());
    REQUIRE_FALSE(defaultKeyState.isActive());
}

TEST_CASE("KeyState constructor with pressed value") {
    KeyState pressedKeyState{1};
    REQUIRE_FALSE(pressedKeyState.isReleased());
    REQUIRE(pressedKeyState.isPressed());
    REQUIRE_FALSE(pressedKeyState.isHeld());
    REQUIRE(pressedKeyState.isActive());
}

TEST_CASE("KeyState constructor with held value") {
    KeyState heldKeyState{2};
    REQUIRE_FALSE(heldKeyState.isReleased());
    REQUIRE_FALSE(heldKeyState.isPressed());
    REQUIRE(heldKeyState.isHeld());
    REQUIRE(heldKeyState.isActive());
}

TEST_CASE("KeyState constructor with released value") {
    KeyState releasedKeyState{0};
    REQUIRE(releasedKeyState.isReleased());
    REQUIRE_FALSE(releasedKeyState.isPressed());
    REQUIRE_FALSE(releasedKeyState.isHeld());
    REQUIRE_FALSE(releasedKeyState.isActive());
}

TEST_CASE("KeyState constructor with invalid value defaults to released") {
    KeyState invalidKeyState{999};
    REQUIRE(invalidKeyState.isReleased());
    REQUIRE_FALSE(invalidKeyState.isActive());
}

TEST_CASE("KeyState isActive function works correctly") {
    KeyState pressedKeyState{1};
    KeyState heldKeyState{2};
    KeyState defaultKeyState;
    KeyState releasedKeyState{0};

    REQUIRE(pressedKeyState.isActive());
    REQUIRE(heldKeyState.isActive());
    REQUIRE_FALSE(defaultKeyState.isActive());
    REQUIRE_FALSE(releasedKeyState.isActive());
}

TEST_CASE("KeyState equality operator") {
    KeyState state1{1};
    KeyState state2{1};
    KeyState state3{2};
    KeyState defaultKeyState;
    KeyState releasedKeyState{0};

    REQUIRE(state1 == state2);
    REQUIRE_FALSE(state1 == state3);
    REQUIRE(defaultKeyState == releasedKeyState);
}

TEST_CASE("KeyState conversion to string_view") {
    KeyState defaultKeyState;
    KeyState pressedKeyState{1};
    KeyState heldKeyState{2};

    REQUIRE(std::string_view(defaultKeyState) == "released");
    REQUIRE(std::string_view(pressedKeyState) == "pressed");
    REQUIRE(std::string_view(heldKeyState) == "held");
}

TEST_CASE("KeyState stream output operator") {
    KeyState defaultKeyState;
    KeyState pressedKeyState{1};
    KeyState heldKeyState{2};

    SECTION("released state") {
        std::stringstream ss;
        ss << defaultKeyState;
        REQUIRE(ss.str() == "released");
    }

    SECTION("pressed state") {
        std::stringstream ss;
        ss << pressedKeyState;
        REQUIRE(ss.str() == "pressed");
    }

    SECTION("held state") {
        std::stringstream ss;
        ss << heldKeyState;
        REQUIRE(ss.str() == "held");
    }
}

TEST_CASE("KeyState conversion to integral") {
    KeyState defaultKeyState;
    KeyState pressedKeyState{1};
    KeyState heldKeyState{2};

    REQUIRE(static_cast<int>(defaultKeyState) ==
            static_cast<int>(KeyStateEnum::RELEASED));
    REQUIRE(static_cast<int>(pressedKeyState) ==
            static_cast<int>(KeyStateEnum::PRESSED));
    REQUIRE(static_cast<int>(heldKeyState) ==
            static_cast<int>(KeyStateEnum::HELD));
}

TEST_CASE("KeyState transitions") {
    KeyState state;

    REQUIRE(state.isReleased());

    state = KeyState{1};
    REQUIRE(state.isPressed());

    state = KeyState{2};
    REQUIRE(state.isHeld());

    state = KeyState{0};
    REQUIRE(state.isReleased());
}

TEST_CASE("KeyState multiple state changes") {
    KeyState state;

    for (int i = 0; i < 3; i++) {
        state = KeyState{1}; // pressed
        REQUIRE(state.isActive());

        state = KeyState{2}; // held
        REQUIRE(state.isActive());

        state = KeyState{0}; // released
        REQUIRE_FALSE(state.isActive());
    }
}
