#include "event_device.h"
#include "keystate.h"
#include <argparse/argparse.hpp>
#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>

void soundEvent(KeyState state, int sound_number) {
    if (state.isPressed()) {
        std::cout << std::format("Play sound {}", sound_number) << std::endl;
    } else if (state.isReleased()) {
        std::cout << std::format("Stop sound {}", sound_number) << std::endl;
    }
}

void volumeEvent(int step) {
    static int volume = 50;
    volume += step;
    if (volume < 0)
        volume = 0;
    if (volume > 100)
        volume = 100;
    std::cout << std::format("Volume is now {}", volume) << std::endl;
}

void bankEvent(int bankNumber) {
    std::cout << std::format("Bank {} selected", bankNumber) << std::endl;
}

void stopEvent(KeyState state, bool *exitSignal) {
    // Amount of time the stop key must be held to trigger a shutdown event
    constexpr auto shutdownHoldTime = std::chrono::seconds(3);
    // Static variable that saves previous time the stop key was pressed
    static auto lastStopPress = std::chrono::steady_clock::time_point{};

    if (state.isPressed()) {
        lastStopPress = std::chrono::steady_clock::now();
        std::cout << "Stop event" << std::endl;
    } else if (state.isHeld()) {
        // Shutdown event if stop key was held for long enough
        auto holdDuration = std::chrono::steady_clock::now() - lastStopPress;
        if (holdDuration > shutdownHoldTime &&
            lastStopPress != std::chrono::steady_clock::time_point{}) {
            std::cout << "Shutdown event" << std::endl;
            lastStopPress = std::chrono::steady_clock::time_point{};
            *exitSignal = true;
        }
    } else if (state.isReleased()) {
        lastStopPress = std::chrono::steady_clock::time_point{};
    }
}

void changeReleaseMode(EventDevice &device) {
    static bool releaseMode = false;
    releaseMode = !releaseMode;
    if (releaseMode) {
        device.setLed(1, true);
        std::cout << "Release mode on" << std::endl;
    } else {
        device.setLed(1, false);
        std::cout << "Release mode off" << std::endl;
    }
}

/**
 * Handle a key event
 */
void process_key(EventDevice &device, bool *exitSignal, key_event_t keyEvent) {
    auto [code, state] = keyEvent;

    // Led 0 for normal keys, led 2 for fn key
    if (code == fnKey) {
        device.setLed(2, state.isActive());
    } else {
        device.setLed(0, state.isActive());
    }

    // Check if the key matches any of our events and trigger them
    if (soundKeys.contains(code) && !device.keyIsActive(fnKey)) {
        soundEvent(state, soundKeys.at(code));
    } else if (volumeKeys.contains(code) && device.keyIsActive(fnKey) &&
               state.isActive()) {
        volumeEvent(volumeKeys.at(code));
    } else if (bankKeys.contains(code) && device.keyIsActive(fnKey) &&
               state.isPressed()) {
        bankEvent(bankKeys.at(code));
    } else if (code == stopKey && !device.keyIsActive(fnKey)) {
        stopEvent(state, exitSignal);
    } else if (code == stopKey && device.keyIsActive(fnKey) &&
               state.isPressed()) {
        changeReleaseMode(device);
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
        bool exitSignal = false;
        while (!exitSignal) {
            auto key_event = device.getNextKey();
            process_key(device, &exitSignal, key_event);
        }
        device.cycleLeds();
    } catch (const std::exception &exc) {
        std::cerr << exc.what() << std::endl;
    }

    return 0;
}
