#include "commands.h"
#include "event_device.h"
#include "keystate.hpp"
#include <argparse/argparse.hpp>
#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>

enum EventLed { NORMAL_KEY, LOOP_MODE, FN_KEY };
using name_file_map_t = std::map<std::string, std::filesystem::path>;

std::optional<int> getNewVolume(int step) {
    static int volume = 50;
    int newVolume = volume + step;
    if (newVolume < 0)
        newVolume = 0;
    if (newVolume > 100)
        newVolume = 100;

    if (newVolume == volume) {
        return std::nullopt;
    }
    volume = newVolume;
    return volume;
}

bool getNewReleaseMode() {
    static bool releaseMode = false;
    releaseMode = !releaseMode;
    return releaseMode;
}

bool isShutdownEvent(KeyState state) {
    // Amount of time the stop key must be held to trigger a shutdown event
    constexpr auto shutdownHoldTime = std::chrono::seconds(3);
    // Static variable that saves previous time the stop key was pressed
    static auto lastStopPress = std::chrono::steady_clock::time_point{};

    if (state.isPressed()) {
        lastStopPress = std::chrono::steady_clock::now();
    } else if (state.isHeld()) {
        // Shutdown event if stop key was held for long enough
        auto holdDuration = std::chrono::steady_clock::now() - lastStopPress;
        if (holdDuration > shutdownHoldTime &&
            lastStopPress != std::chrono::steady_clock::time_point{}) {
            lastStopPress = std::chrono::steady_clock::time_point{};
            return true;
        }
    } else if (state.isReleased()) {
        lastStopPress = std::chrono::steady_clock::time_point{};
    }
    return false;
}

/**
 * Set new led based on key event
 */
void handleKeyLeds(EventDevice &device, key_event_t keyEvent) {
    // Led 0 for normal keys, led 2 for fn key
    auto [code, state] = keyEvent;
    if (code == fnKey) {
        device.setLed(EventLed::FN_KEY, state.isActive());
    } else {
        device.setLed(EventLed::NORMAL_KEY, state.isActive());
    }
}

/**
 * Read a key event and return the corresponding command, if any
 */
std::optional<Command> parse_command(EventDevice &device,
                                     key_event_t keyEvent) {
    auto [code, state] = keyEvent;

    // Check if the key matches any of our events and trigger them
    if (soundKeys.contains(code) && !device.keyIsActive(fnKey)) {
        // Sound event
        auto sound_number = soundKeys.at(code);
        if (state.isPressed()) {
            return Command(CommandType::PLAY_SOUND, sound_number);
        } else if (state.isReleased()) {
            return Command(CommandType::STOP_SOUND, sound_number);
        }
    } else if (volumeKeys.contains(code) && device.keyIsActive(fnKey) &&
               state.isActive()) {
        // Volume event
        auto newVolume = getNewVolume(volumeKeys.at(code));
        if (newVolume.has_value()) {
            return Command(CommandType::VOLUME, newVolume.value());
        }
    } else if (bankKeys.contains(code) && device.keyIsActive(fnKey) &&
               state.isPressed()) {
        // Bank event
        auto bankNumber = bankKeys.at(code);
        return Command(CommandType::NEW_BANK, bankNumber);
    } else if (code == stopKey && !device.keyIsActive(fnKey)) {
        // Stop event
        if (isShutdownEvent(state)) {
            return Command(CommandType::EXIT);
        } else if (state.isPressed()) {
            return Command(CommandType::STOP_SOUND);
        }
    } else if (code == stopKey && device.keyIsActive(fnKey) &&
               state.isPressed()) {
        // Release mode toggle event
        auto releaseMode = getNewReleaseMode();
        return Command(releaseMode ? CommandType::LOOP_ON
                                   : CommandType::LOOP_OFF);
    }
    // No event, return empty optional
    return std::nullopt;
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
            std::cerr << std::format("Could not open input event file '{}': {}",
                                     device_path.string(), exception.what())
                      << std::endl;
        }
    }
    return name_file_map;
}

void keyMonitorLoop(EventDevice &device) {
    device.cycleLeds();
    bool exitLoop = false;
    while (!exitLoop) {
        key_event_t keyEvent;
        try {
            keyEvent = device.getNextKey();
        } catch (const std::runtime_error &exc) {
            std::cerr << std::format("Error while reading key event: {}",
                                     exc.what())
                      << std::endl;
            continue;
        }

        handleKeyLeds(device, keyEvent);

        auto maybeCommand = parse_command(device, keyEvent);
        if (!maybeCommand) {
            continue;
        }
        auto command = *maybeCommand;

        // Led events for specific commands
        switch (command.getType()) {
        // No led events for these commands
        case CommandType::PLAY_SOUND:
        case CommandType::STOP_SOUND:
        case CommandType::NEW_BANK:
        case CommandType::VOLUME:
            break;
        case CommandType::LOOP_ON:
            device.setLed(EventLed::LOOP_MODE, true);
            break;
        case CommandType::LOOP_OFF:
            device.setLed(EventLed::LOOP_MODE, false);
            break;
        case CommandType::EXIT:
            device.cycleLeds();
            exitLoop = true;
            break;
        }

        send_command(command);
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
        } else {
            device_file = device_file_it->second;
        }
    }

    auto device = EventDevice(device_file);
    std::cerr << std::format("Opened input device {} at file '{}'",
                             device.getName(), device_file.string())
              << std::endl;
    keyMonitorLoop(device);

    return 0;
}
