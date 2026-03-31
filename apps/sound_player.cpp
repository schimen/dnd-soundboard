#include <argparse/argparse.hpp>
#include <filesystem>
#include <format>
#include <iostream>
#include <ranges>
#include <regex>
#include <string>

// Temporarily store path to sound files. Store sound chunk when implementing
// SDL2.
using sound_map_t = std::map<int, std::filesystem::path>;

sound_map_t loadBank(std::filesystem::path bankDir) {
    sound_map_t sounds;
    if (!std::filesystem::exists(bankDir) ||
        !std::filesystem::is_directory(bankDir)) {
        std::cerr << std::format("Path {} is not a directory", bankDir.string())
                  << std::endl;
        return sounds;
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
        sounds[sample] = path;
    }
    return sounds;
}

void startSoundPlayer(std::filesystem::path sampleDir) {
    constexpr int numBanks = 6;
    static_assert(numBanks > 0, "There must be at least one bank");

    // Create a path for each bank and load the first bank
    std::array<std::filesystem::path, numBanks> bankPaths;
    for (const auto bankIndex : std::views::iota(0, numBanks)) {
        bankPaths[bankIndex] =
            sampleDir / std::format("bank_{}", bankIndex + 1);
    }
    auto sounds = loadBank(bankPaths[0]);
    for (const auto &[sample, path] : sounds) {
        std::cout << std::format("Found sample {} at {}", sample, path.string())
                  << std::endl;
    }

    // Listen for stdinput and prin it out for testing
    std::string input_line;
    std::cout << "Sound player" << std::endl;
    while (std::cin) {
        getline(std::cin, input_line);
        std::cout << input_line << std::endl;
    };
};

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
    std::filesystem::path sampleDir = program.get("--sample-dir");
    startSoundPlayer(sampleDir);

    return 0;
}
