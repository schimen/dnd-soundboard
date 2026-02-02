#include <argparse/argparse.hpp>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <iostream>
#include <libevdev/libevdev.h>
#include <map>
#include <unistd.h>
#include <vector>

using name_file_map_t = std::map<std::string, std::filesystem::path>;

class EventDevice {
  public:
    EventDevice(std::filesystem::path device_path);
    ~EventDevice();
    void cycleLeds();
    std::string getName() { return libevdev_get_name(dev); }

  private:
    int fd = -1;
    struct libevdev *dev = NULL;
};

/**
 * Open input event device, or throw exception in case of an error
 */
EventDevice::EventDevice(std::filesystem::path device_path) {
    if (!std::filesystem::exists(device_path)) {
        throw std::invalid_argument(
            std::format("File '{}' does not exist", device_path.string()));
    }
    fd = open(device_path.c_str(), O_RDONLY | O_NONBLOCK);
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

EventDevice::~EventDevice() {
    if (dev != NULL) {
        libevdev_free(dev);
    }
    if (fd >= 0) {
        close(fd);
    }
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
 * Open all event files and create a map with the device name as key and file
 * path as value
 */
name_file_map_t get_input_files_by_names() {
    name_file_map_t name_file_map;
    for (const auto &entry :
         std::filesystem::directory_iterator("/dev/input/")) {
        auto device_path = entry.path();
        if (!device_path.filename().string().contains("event")) {
            // We are only interested in event input devices
            continue;
        }
        try {
            auto device = EventDevice(device_path);
            name_file_map[device.getName()] = device_path;
        } catch (const std::exception &exc) {
            std::cerr << exc.what() << std::endl;
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

    // Get path to device file
    std::filesystem::path device_file;
    if (program.is_used("--device-file")) {
        device_file = program.get("--device-file");
    } else {
        auto device_name = program.get("--device-name");
        auto name_files_map = get_input_files_by_names();
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
    } catch (const std::exception &exc) {
        std::cerr << exc.what() << std::endl;
    }
    return 0;
}
