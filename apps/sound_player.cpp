#include "commands.h"
#include "sample.h"
#include <SDL3_mixer/SDL_mixer.h>
#include <argparse/argparse.hpp>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <ranges>
#include <regex>
#include <string>
#include <thread>

using bank_map_t = std::map<int, std::filesystem::path>;
using queue_map_t = std::map<int, CommandQueue>;
using thread_map_t = std::map<int, std::thread>;
using sample_vector_t = std::vector<Sample>;
using bank_path_vector_t = std::vector<std::filesystem::path>;

constexpr int numSounds = 6;
constexpr int numBanks = 4;
static_assert(numBanks > 0, "There must be at least one bank");

/**
 * Read the bank directory and return the number of each existing sample and its
 * path.
 */
bank_map_t readBank(std::filesystem::path bankDir) {
    bank_map_t bank;
    if (!std::filesystem::exists(bankDir) ||
        !std::filesystem::is_directory(bankDir)) {
        std::cerr << std::format("Path {} is not a directory", bankDir.string())
                  << std::endl;
        return bank;
    }

    // Regex pattern for sound names: Match for number at start of filename,
    // ignore any name in the middle and allow either .wav or .mp3 extension
    std::regex sound_pattern{R"((\d).*\.(wav|mp3))"};

    for (const auto &entry : std::filesystem::directory_iterator(bankDir)) {
        std::smatch matches;
        auto path = entry.path();
        auto sound_name = path.filename().string();
        bool match_found = std::regex_match(sound_name, matches, sound_pattern);
        if (!match_found) {
            continue;
        }
        if (matches.size() != 3) {
            std::cerr << std::format("File {} matched sound pattern but did "
                                     "not have the expected number of groups",
                                     sound_name)
                      << std::endl;
            continue;
        }
        int sample = std::stoi(matches[1]);
        if (sample < 1 || sample > numSounds) {
            continue;
        }
        bank[sample] = path;
    }
    return bank;
}

void playSoundLoop(Sample *sample, CommandQueue *queue) {
    while (true) {
        auto command = queue->get();
        auto type = command.getType();
        if (type == CommandType::PLAY_SOUND) {
            sample->play();
        } else if (type == CommandType::STOP_SOUND) {
            sample->stop();
        } else if (type == CommandType::LOOP_ON ||
                   type == CommandType::LOOP_OFF) {
            sample->setLoopMode(type == CommandType::LOOP_ON);
        } else if (type == CommandType::EXIT) {
            break;
        }
    }
    sample->closeSample();
}

void processCommand(const Command command, queue_map_t &queues) {
    CommandType type = command.getType();
    std::optional<int> arg = command.getArg();

    if (type == CommandType::PLAY_SOUND) {
        if (!arg) {
            std::cerr << "Play sound command got no valid arguments"
                      << std::endl;
            return;
        }
        int sample_id = *arg;
        if (queues.contains(sample_id)) {
            queues[sample_id].put(command);
        }
    } else if (type == CommandType::STOP_SOUND) {
        if (!arg) {
            // No argument, stop all sounds
            for (auto &queue : std::views::values(queues)) {
                queue.put(command);
            }
        } else {
            // Only stop a single sound
            int sample = *arg;
            if (queues.contains(sample)) {
                queues[sample].put(command);
            }
        }
    } else if (type == CommandType::VOLUME) {
        if (!arg) {
            std::cerr << "Volume command got no valid arguments" << std::endl;
            return;
        }
        std::cout << std::format("Volume command got argument: {}", *arg)
                  << std::endl;
    } else if (type == CommandType::LOOP_ON || type == CommandType::LOOP_OFF) {
        // Set new loop state in all queues
        for (auto &queue : std::views::values(queues)) {
            queue.put(command);
        }
    }
}

void openQueues(queue_map_t &queues, const bank_map_t &bank) {
    // Remove queues for samples that are no longer in the bank
    std::erase_if(queues, [&bank](const auto &entry) {
        return !bank.contains(entry.first);
    });
    // Clear all existing queues
    for (auto &queue : std::views::values(queues)) {
        queue.clear();
    }
    // Create an empty queue for each sample where none exist
    for (const auto sample : std::views::keys(bank)) {
        if (!queues.contains(sample)) {
            queues[sample];
        }
    }
}

void openThreads(thread_map_t &threads, queue_map_t &queues,
                 const bank_map_t &bank, sample_vector_t &samples) {
    for (const auto &[sample_id, path] : bank) {
        if (sample_id < 1 || sample_id > numSounds) {
            std::cerr << "Sample id " << sample_id << " is out of range"
                      << std::endl;
            continue;
        } else if (threads.contains(sample_id) || !queues.contains(sample_id)) {
            std::cerr << "Thread for sample " << sample_id
                      << " already exists or "
                         "no queue found, skipping thread creation"
                      << std::endl;
            continue;
        }

        CommandQueue *queue = &queues[sample_id];
        Sample *sample = &samples[sample_id - 1];
        try {
            sample->openSample(path);
        } catch (const std::exception &exc) {
            std::cerr << std::format(
                             "Error while opening sound {} from file {}: {}",
                             sample->getId(), path.string(), exc.what())
                      << std::endl;
            continue;
        }
        threads[sample_id] = std::thread(playSoundLoop, sample, queue);
    }
}

void exitThreads(thread_map_t &threads, queue_map_t &queues) {
    for (auto &queue : std::views::values(queues)) {
        queue.put(Command(CommandType::STOP_SOUND));
        queue.put(Command(CommandType::EXIT));
    }
    for (auto &thread : std::views::values(threads)) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads.clear();
}

// Create sample object for each sound
sample_vector_t initSamples(MIX_Mixer *mixer) {
    sample_vector_t samples;
    samples.reserve(numSounds);
    for (const auto sampleId : std::views::iota(1, numSounds + 1)) {
        samples.emplace_back(sampleId, mixer);
    }
    return samples;
}

// Create a path for each bank
bank_path_vector_t getBankPaths(std::filesystem::path sampleDir) {
    bank_path_vector_t bankPaths(numBanks);
    for (const auto bankIndex : std::views::iota(0, numBanks)) {
        bankPaths[bankIndex] =
            sampleDir / std::format("bank_{}", bankIndex + 1);
    }
    return bankPaths;
}

void startSoundPlayer(std::filesystem::path sampleDir, MIX_Mixer *mixer) {
    thread_map_t threads;
    queue_map_t queues;
    sample_vector_t samples = initSamples(mixer);
    bank_path_vector_t bankPaths = getBankPaths(sampleDir);
    bool loopMode = false;
    auto bank = readBank(bankPaths[0]);

    // Open a thread and command queue for each sample in the bank
    openQueues(queues, bank);
    openThreads(threads, queues, bank, samples);

    // Listen for stdinput and print it out for testing
    std::string input_line;
    while (std::cin) {
        auto maybe_command = receive_command();
        if (!maybe_command) {
            std::cerr << "Invalid command" << std::endl;
            continue;
        }
        auto command = *maybe_command;
        auto type = command.getType();
        std::optional<int> arg = command.getArg();

        if (type == CommandType::EXIT) {
            std::cout << "Exit command received, exiting sound player"
                      << std::endl;
            exitThreads(threads, queues);
            queues.clear();
            break;
        } else if (type == CommandType::NEW_BANK) {
            if (!arg) {
                std::cerr << "Bank command got no arguments" << std::endl;
                continue;
            }
            size_t bankIndex = *arg - 1;
            if (bankIndex >= bankPaths.size()) {
                std::cerr << std::format(
                                 "Bank command got invalid bank index: {}",
                                 *arg)
                          << std::endl;
                continue;
            }
            exitThreads(threads, queues);
            bank = readBank(bankPaths[bankIndex]);
            openQueues(queues, bank);
            openThreads(threads, queues, bank, samples);
            processCommand(loopMode ? Command(CommandType::LOOP_ON)
                                    : Command(CommandType::LOOP_OFF),
                           queues);
        } else {
            if (type == CommandType::LOOP_ON) {
                loopMode = true;
            } else if (type == CommandType::LOOP_OFF) {
                loopMode = false;
            }
            processCommand(command, queues);
        }
    }
    // Exit  all threads here in case any thread is still running
    exitThreads(threads, queues);
}

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("sound_player");
    program.add_argument("--sample-dir")
        .default_value("samples")
        .nargs(1)
        .help("Directory with samples");
    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }
    if (!MIX_Init()) {
        std::cerr << "Couldn't init SDL_mixer library: " << SDL_GetError()
                  << std::endl;
        return 1;
    }

    std::filesystem::path sampleDir = program.get("--sample-dir");

    MIX_Mixer *mixer =
        MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (!mixer) {
        std::cerr << std::format("Couldn't create mixer device: {}",
                                 SDL_GetError())
                  << std::endl;
        return 1;
    }

    try {
        startSoundPlayer(sampleDir, mixer);
    } catch (const std::runtime_error &exc) {
        std::cerr << std::format("Error while running sound player: {}",
                                 exc.what())
                  << std::endl;
    }
    MIX_Quit();

    return 0;
}
