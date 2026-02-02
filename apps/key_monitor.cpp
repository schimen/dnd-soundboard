#include <argparse/argparse.hpp>
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <iostream>
#include <libevdev/libevdev.h>
#include <unistd.h>
#include <vector>

/**
 * Open all event files and return the first that contains the specified name
 */
std::filesystem::path get_file_by_name(std::string name) {
    struct libevdev *dev = NULL;
    int fd, rc;
    for (const auto &entry :
         std::filesystem::directory_iterator("/dev/input/")) {
        auto device_path = entry.path();
        fd = open(device_path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            // We should be able to open all files here, warn about this
            std::cerr << std::format("Could not open file '{}'",
                                     device_path.string())
                      << std::endl;
            continue;
        }
        rc = libevdev_new_from_fd(fd, &dev);
        if (rc < 0) {
            // Probably not a valid device file, keep searching
            close(fd);
            continue;
        }
        std::string device_name = libevdev_get_name(dev);
        libevdev_free(dev);
        close(fd);
        if (device_name.contains(name)) {
            return device_path;
        }
    }
    std::cerr << std::format(
                     "Could not find any input device with the name '{}'", name)
              << std::endl;
    return std::filesystem::path();
}

/**
 * Cycle the leds on the keyboard on and off
 */
void cycle_leds(struct libevdev *dev) {
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
        device_file = get_file_by_name(program.get("--device-name"));
    }
    if (!std::filesystem::exists(device_file)) {
        std::cerr << std::format("Device file '{}' does not exist",
                                 device_file.string())
                  << std::endl;
        return 1;
    }

    struct libevdev *dev = NULL;
    int fd, rc;
    fd = open(device_file.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        std::cerr << std::format("Failed to open event file '{}'",
                                 device_file.string())
                  << std::endl;
    }
    rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0) {
        std::cerr << std::format("Failed to create event device from file '{}'",
                                 device_file.string())
                  << std::endl;
        return 1;
    }
    std::cout << std::format("Opened input device {} at file '{}'",
                             libevdev_get_name(dev), device_file.string())
              << std::endl;
    cycle_leds(dev);
    libevdev_free(dev);
    close(fd);
    return 0;
}
