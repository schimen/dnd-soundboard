#include "event_device.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <thread>

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

void EventDevice::cycleLeds() {
    constexpr auto ledPause = std::chrono::milliseconds(100);
    for (unsigned int i = 0; i < ledState.size(); i++) {
        setLedValue(i, true);
        std::this_thread::sleep_for(ledPause);
    }
    for (unsigned int i = 0; i < ledState.size(); i++) {
        setLedValue(i, false);
        std::this_thread::sleep_for(ledPause);
    }
}

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
            // We got a valid key, save it and exit loop
            auto code = event_struct.code;
            auto state = KeyState(event_struct.value);
            newKey = {code, state};
            lastKey = newKey;
            if (state.isActive()) {
                activeKeys.insert(code);
            } else if (activeKeys.contains(code)) {
                activeKeys.erase(code);
            }
            break;
        }
    }
    return newKey;
}

void EventDevice::closeDevice() {
    if (dev != NULL) {
        libevdev_free(dev);
    }
    if (fd >= 0) {
        close(fd);
    }
};

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

void EventDevice::setLed(size_t ledno, bool state) {
    if (ledno >= ledState.size())
        return;
    if (ledState[ledno] == state)
        return;

    ledState[ledno] = state;
    libevdev_kernel_set_led_value(dev, keyboardLedValues[ledno],
                                  state ? LIBEVDEV_LED_ON : LIBEVDEV_LED_OFF);
}
