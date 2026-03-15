#include "key_monitor.h"
#include <algorithm>
#include <argparse/argparse.hpp>
#include <bitset>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <thread>

/**
 * Open input event device, or throw exception in case of an error
 */
EventDevice::EventDevice(std::filesystem::path device_path) {
    if (!std::filesystem::exists(device_path)) {
        throw std::invalid_argument(
            std::format("File '{}' does not exist", device_path.string()));
    }
    fd = open(device_path.c_str(), O_RDWR);
    if (fd < 0) {
        throw std::runtime_error(
            std::format("Could not open file '{}'", device_path.string()));
    }
    int rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0) {
        close(fd);
        fd = -1;
        throw std::runtime_error(
            std::format("Error {} when opening event device '{}'",
                        strerror(errno), device_path.string()));
    }
};

EventDevice::~EventDevice() { closeDevice(); }

EventDevice::EventDevice(EventDevice &&other)
    : fd(other.fd), dev(other.dev), readFlag(other.readFlag) {
    // Reset references in the old object
    other.resetReferences();
}

EventDevice &EventDevice::operator=(EventDevice &&other) {
    if (this != &other) {
        // Clean up previous device
        closeDevice();

        // Take ownership of other object resources
        fd = other.fd;
        dev = other.dev;
        readFlag = other.readFlag;

        // Reset references in the old object
        other.resetReferences();
    }
    return *this;
}

/**
 * Cycle the leds on the keyboard on and off
 */
void EventDevice::cycleLeds() {
    constexpr auto ledPause = std::chrono::milliseconds(100);
    for (unsigned int i = 0; i < ledState.size(); i++) {
        setLed(i, true);
        std::this_thread::sleep_for(ledPause);
    }
    for (unsigned int i = 0; i < ledState.size(); i++) {
        setLed(i, false);
        std::this_thread::sleep_for(ledPause);
    }
}

/**
 * Blocking wait for the next input event and return it
 */
key_event_t EventDevice::getNextKey() {
    int rc;
    struct input_event event_struct;
    key_event_t newKey;

    while (true) {
        // Read event and check the return code
        rc = libevdev_next_event(dev, readFlag, &event_struct);
        switch (rc) {
        case -EAGAIN:
            // No event, try again
            continue;
        case LIBEVDEV_READ_STATUS_SYNC:
            // We lost a message, synchronize the next time we read
            readFlag = LIBEVDEV_READ_FLAG_SYNC;
            break;
        case LIBEVDEV_READ_STATUS_SUCCESS:
            // Event read was a success, read normally next time
            readFlag = LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING;
            break;
        default:
            // We got a return code that we could not handle
            throw std::runtime_error(
                std::format("Failed to handle events: {}", strerror(-rc)));
        }

        if (event_struct.type == EV_LED) {
            // Make sure leds behave correct
            handleLedEvent(event_struct.code, event_struct.value != 0);
        } else if (event_struct.type == EV_KEY) {
            auto code = event_struct.code;
            auto state = getKeyState(event_struct.value);
            newKey = {code, state};
            if (lastKey == newKey) {
                // Ignore if this event was the same as the previous
                continue;
            }
            // We got a valid key, save it and exit loop
            lastKey = newKey;
            if (isActive(state)) {
                activeKeys.insert(code);
            } else if (activeKeys.contains(code)) {
                activeKeys.erase(code);
            }
            break;
        }
    }
    return newKey;
}

/**
 * Check if key code is in list of currently active keys
 */
bool EventDevice::keyIsActive(key_code_t code) {
    auto activeKeyIt = std::find(activeKeys.begin(), activeKeys.end(), code);
    return activeKeyIt != activeKeys.end();
}

/**
 * Free evdev memory and close the input event file
 */
void EventDevice::closeDevice() {
    if (dev != NULL) {
        libevdev_free(dev);
    }
    if (fd >= 0) {
        close(fd);
    }
};

/**
 * Handle a led event. If the led is tracked and not of the state we expect, set
 * it to the expected value
 */
void EventDevice::handleLedEvent(key_code_t code, bool value) {
    size_t ledno = std::distance(
        keyboardLedValues.begin(),
        std::find(keyboardLedValues.begin(), keyboardLedValues.end(), code));
    if (ledno >= keyboardLedValues.size()) {
        return;
    }
    bool expectedState = ledState[ledno];
    if (value != expectedState) {
        setLed(ledno, expectedState);
    }
}

/**
 * Set the led value on the keyboard and save the new expected led value
 */
void EventDevice::setLed(size_t ledno, bool state) {
    if (ledno >= ledState.size())
        return;
    ledState[ledno] = state;
    libevdev_kernel_set_led_value(dev, keyboardLedValues[ledno],
                                  state ? LIBEVDEV_LED_ON : LIBEVDEV_LED_OFF);
}

/**
 * Convert the value of a input event to a key state
 */
key_state_t getKeyState(std::integral auto value) {
    switch (value) {
    case 1:
        return KEY_PRESSED;
    case 2:
        return KEY_HOLD;
    default:
        return KEY_RELEASED;
    }
};

/**
 * Take key that changes volume and return new volume level
 */
int changeVolume(key_code_t code) {
    static int volume = 50;
    if (!volumeKeys.contains(code)) {
        return volume;
    }
    auto volumeChange = volumeKeys.at(code);
    volume += volumeChange;
    if (volume < 0)
        volume = 0;
    if (volume > 100)
        volume = 100;
    return volume;
}

/**
 * Handle a key event
 */
void process_key(EventDevice &device, key_event_t keyEvent) {
    // Amount of time the stop key must be held to trigger a shutdown event
    // constexpr auto shutdownHoldTime = std::chrono::seconds(3);

    auto [code, state] = keyEvent;
    std::map<size_t, bool> ledEvents;

    if (code == fnKey) {
        ledEvents[2] = isActive(state);
    }

    if (soundKeys.contains(code) && !device.keyIsActive(fnKey)) {
        // Sound event
        std::cout << std::format("Sound {} {}", soundKeys.at(code),
                                 isActive(state) ? "active" : "released")
                  << std::endl;
    } else if (volumeKeys.contains(code) && device.keyIsActive(fnKey)) {
        // Volume change
        ledEvents[0] = isActive(state);
        if (isPressed(state)) {
            int volume = changeVolume(code);
            std::cout << std::format("Volume is now {}", volume) << std::endl;
        }
    } else if (bankKeys.contains(code) && device.keyIsActive(fnKey)) {
        // Bank event
        ledEvents[0] = isActive(state);
        if (isPressed(state)) {
            auto bankNumber = bankKeys.at(code);
            std::cout << std::format("Bank {} selected", bankNumber)
                      << std::endl;
        }
    }

    // Set all new led states
    for (const auto &[ledno, state] : ledEvents) {
        device.setLed(ledno, state);
    }
}

/**
 * Open all event files and create a map with the device name as key and file
 * path as value
 */
name_file_map_t get_input_files_by_names() {
    name_file_map_t name_file_map;

    // Open /dev/input/ directory
    std::filesystem::directory_iterator filesystem_it;
    const std::filesystem::path input_dir{"/dev/input/"};
    try {
        filesystem_it = std::filesystem::directory_iterator(input_dir);
    } catch (const std::filesystem::filesystem_error &exception) {
        std::cerr
            << std::format(
                   "Could not open {} to search for input event files: {}",
                   input_dir.string(), exception.what())
            << std::endl;
        return name_file_map;
    }

    for (const auto &entry : filesystem_it) {
        auto device_path = entry.path();
        if (!device_path.filename().string().contains("event")) {
            // We are only interested in event input devices
            continue;
        }
        try {
            auto device = EventDevice(device_path);
            name_file_map[device.getName()] = device_path;
        } catch (const std::exception &exception) {
            std::cerr << exception.what() << std::endl;
        }
    }
    return name_file_map;
}

int main(int argc, char *argv[]) {

    argparse::ArgumentParser program("key_monitor");
    program.add_argument("--device-file")
        .default_value(std::filesystem::path{""})
        .nargs(1)
        .help(
            "Path to input event file for button device. If this option is not "
            "set, device-name is used instead.");
    program.add_argument("--device-name")
        .default_value(std::string{"SEMICO GXT 860 Keyboard"})
        .nargs(1)
        .help("Part of the name of the input event file, will use first file "
              "with name matching argument. This option is ignored if "
              "device-file is set.");
    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    std::filesystem::path device_file;
    if (program.is_used("--device-file")) {
        // Use provided device file
        device_file = program.get("--device-file");
    } else {
        // Find device file by name
        auto device_name = program.get("--device-name");
        auto name_files_map = get_input_files_by_names();
        if (name_files_map.empty()) {
            std::cerr << "No input devices found" << std::endl;
            return 1;
        }
        auto device_file_it = name_files_map.find(device_name);
        if (device_file_it == name_files_map.end()) {
            std::cerr << std::format("Could not find device '{}', choose from "
                                     "the following devices:",
                                     device_name)
                      << std::endl;
            for (auto const &[name, _] : name_files_map) {
                std::cerr << "  " << name << std::endl;
            }
            return 1;
        }
        device_file = device_file_it->second;
    }

    try {
        auto device = EventDevice(device_file);
        std::cout << std::format("Opened input device {} at file '{}'",
                                 device.getName(), device_file.string())
                  << std::endl;
        device.cycleLeds();
        while (true) {
            auto key_event = device.getNextKey();
            process_key(device, key_event);
        }
    } catch (const std::exception &exc) {
        std::cerr << exc.what() << std::endl;
    }

    return 0;
}
