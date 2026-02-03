#include <argparse/argparse.hpp>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <iostream>
#include <libevdev/libevdev.h>
#include <map>
#include <memory>
#include <unistd.h>
#include <vector>

using name_file_map_t = std::map<std::string, std::filesystem::path>;

void process_key(unsigned int code, int value) {
    std::cout << std::format("Key {} {}", code, value ? "pressed" : "released")
              << std::endl;
}

void reset_led_state() {
    std::cout << "led state changed, resetting to last known state" << std::endl;
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
    void handleEvent();

  private:
    int fd = -1;
    struct libevdev *dev = NULL;
    std::unique_ptr<struct input_event> event_ptr;

    void closeDevice() {
        if (dev != NULL) {
            libevdev_free(dev);
        }
        if (fd >= 0) {
            close(fd);
        }
    };

    void resetReferences() {
        fd = -1;
        dev = NULL;
    }
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

    event_ptr = std::make_unique<struct input_event>();
};

EventDevice::~EventDevice() { closeDevice(); }

EventDevice::EventDevice(EventDevice &&other)
    : fd(other.fd), dev(other.dev), event_ptr(std::move(other.event_ptr)) {
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
        event_ptr = std::move(other.event_ptr);

        // Reset references in the old object
        other.resetReferences();
    }
    return *this;
}

/**
 * Cycle the leds on the keyboard on and off
 */
void EventDevice::cycleLeds() {
    const std::vector<unsigned int> led_codes = {LED_NUML, LED_CAPSL,
                                                 LED_SCROLLL};
    for (const auto led_code : led_codes) {
        libevdev_kernel_set_led_value(dev, led_code, LIBEVDEV_LED_ON);
        usleep(100000);
    }
    for (const auto led_code : led_codes) {
        libevdev_kernel_set_led_value(dev, led_code, LIBEVDEV_LED_OFF);
        usleep(100000);
    }
}

/**
 * Blocking wait for the next input event and process it.
 */
void EventDevice::handleEvent() {
    int rc = libevdev_next_event(
        dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING,
        event_ptr.get());
    if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
        // Handle event
        if (event_ptr->type == EV_KEY) {
            process_key(event_ptr->code, event_ptr->value);
        } else if (event_ptr->type == EV_LED) {
            reset_led_state();
        }
    } else if (rc == LIBEVDEV_READ_STATUS_SYNC) {
        // Synchronize events
        while (rc == LIBEVDEV_READ_STATUS_SYNC) {
            rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC,
                                     event_ptr.get());
        }
    } else if (rc == -EAGAIN) {
        // No events to read
        return;
    } else {
        throw std::runtime_error(
            std::format("Error handling event: {}", strerror(-rc)));
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
            device.handleEvent();
        }
    } catch (const std::exception &exc) {
        std::cerr << exc.what() << std::endl;
    }

    return 0;
}
