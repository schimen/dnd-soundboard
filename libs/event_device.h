#pragma once

#include "keystate.h"
#include <array>
#include <bitset>
#include <chrono>
#include <filesystem>
#include <libevdev/libevdev.h>
#include <map>
#include <set>
#include <utility>

using key_code_t = decltype(input_event::code);
using key_event_t = std::pair<key_code_t, KeyState>;
using name_file_map_t = std::map<std::string, std::filesystem::path>;

// All tracked keyboard leds
constexpr std::array<key_code_t, 3> keyboardLedValues = {LED_NUML, LED_CAPSL,
                                                         LED_SCROLLL};
// When this key is held, the alternative function for a key is used
constexpr key_code_t fnKey = 74;
// This button sends stop signal
constexpr key_code_t stopKey = 78;
// Keys that represent each sound
const std::map<key_code_t, int> soundKeys = {{69, 1}, {98, 2}, {55, 3},
                                             {71, 4}, {72, 5}, {73, 6}};
// Keys that represent the different sound banks
const std::map<key_code_t, int> bankKeys = {{69, 1}, {98, 2}, {71, 3}, {72, 4}};
// Volume keys and steps
const std::map<key_code_t, int> volumeKeys = {{55, +5}, {73, -5}};

class EventDevice {
  public:
    // Open input event device, or throw exception in case of an error
    EventDevice(std::filesystem::path device_path);
    ~EventDevice();

    // Delete copy constructor and copy assignment
    EventDevice(const EventDevice &) = delete;
    EventDevice &operator=(const EventDevice &) = delete;

    // Move constructor and move assignment
    EventDevice(EventDevice &&other);
    EventDevice &operator=(EventDevice &&other);

    // Cycle the leds on the keyboard on and off
    void cycleLeds();

    // Get name of input device
    std::string getName() { return libevdev_get_name(dev); }

    // Blocking wait for the next input event and return it
    key_event_t getNextKey();

    // Check if key code is one of the currently active keys
    bool keyIsActive(key_code_t code) { return activeKeys.contains(code); };

    // Set the led value on the keyboard and save the new expected led value
    void setLed(size_t ledno, bool state);

  private:
    int fd = -1;
    struct libevdev *dev = NULL;
    unsigned int readFlag =
        LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING;
    std::bitset<keyboardLedValues.size()> ledState;
    std::set<key_code_t> activeKeys;
    key_event_t lastKey{0, KeyState{}};

    // Free evdev memory and close the input event file
    void closeDevice();

    // If the led is tracked and not of the state we expect, set it to the
    // expected value
    void handleLedEvent(key_code_t code, bool value);

    // Internal led setting function
    void setLedValue(size_t ledno, bool state) {
        ledState[ledno] = state;
        libevdev_kernel_set_led_value(dev, keyboardLedValues[ledno],
                                      state ? LIBEVDEV_LED_ON
                                            : LIBEVDEV_LED_OFF);
    }

    // Reset all references to the device
    void resetReferences() {
        fd = -1;
        dev = NULL;
    }
};
