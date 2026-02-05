#include <algorithm>
#include <argparse/argparse.hpp>
#include <array>
#include <bitset>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <iostream>
#include <iterator>
#include <libevdev/libevdev.h>
#include <map>
#include <unistd.h>

using name_file_map_t = std::map<std::string, std::filesystem::path>;
using key_event_t = std::pair<unsigned int, bool>;

constexpr std::array<unsigned int, 3> keyboardLedValues = {LED_NUML, LED_CAPSL,
                                                           LED_SCROLLL};

void process_key(key_event_t keyEvent) {
    auto [code, state] = keyEvent;
    std::cout << std::format("Key {} {}", code, state ? "pressed" : "released")
              << std::endl;
}

class EventDevice {
  public:
    EventDevice(std::filesystem::path device_path);
    ~EventDevice();

    // Delete copy constructor and copy assignment
    EventDevice(const EventDevice &) = delete;
    EventDevice &operator=(const EventDevice &) = delete;

    // Move constructor and move assignment
    EventDevice(EventDevice &&other);
    EventDevice &operator=(EventDevice &&other);

    void cycleLeds();
    std::string getName() { return libevdev_get_name(dev); }
    key_event_t getNextKey();

  private:
    int fd = -1;
    struct libevdev *dev = NULL;
    unsigned int readFlag =
        LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING;
    std::bitset<keyboardLedValues.size()> ledState;
    key_event_t lastKey{0, false};

    void closeDevice();
    void handleLedEvent(unsigned int code, bool value);
    void resetReferences() {
        fd = -1;
        dev = NULL;
    }
    void setLed(size_t ledno, bool state);
};

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
    for (unsigned int i = 0; i < keyboardLedValues.size(); i++) {
        setLed(i, true);
        usleep(100000);
    }
    for (unsigned int i = 0; i < keyboardLedValues.size(); i++) {
        setLed(i, false);
        usleep(100000);
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
            newKey = {event_struct.code, event_struct.value != 0};
            if (lastKey == newKey) {
                // Ignore if this event was the same as the previous
                continue;
            }
            // We got a valid key, exit the loop
            break;
        }
    }
    // Save the key and return it
    lastKey = newKey;
    return newKey;
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
void EventDevice::handleLedEvent(unsigned int code, bool value) {
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
    if (ledno >= keyboardLedValues.size()) {
        return;
    }
    ledState[ledno] = state;
    libevdev_kernel_set_led_value(dev, keyboardLedValues[ledno],
                                  state ? LIBEVDEV_LED_ON : LIBEVDEV_LED_OFF);
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
            process_key(key_event);
        }
    } catch (const std::exception &exc) {
        std::cerr << exc.what() << std::endl;
    }

    return 0;
}
