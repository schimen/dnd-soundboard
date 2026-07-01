#include "sample.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_properties.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <cmath>
#include <filesystem>
#include <iostream>

Sample::Sample(int id, MIX_Mixer *mixer) : id(id), mixer(mixer) {
    track = MIX_CreateTrack(mixer);
    if (!track) {
        throw std::runtime_error(
            std::format("Couldn't create a mixer track: {}", SDL_GetError()));
    }

    options = SDL_CreateProperties();
    if (!options) {
        throw std::runtime_error(
            std::format("Couldn't create play options: {}", SDL_GetError()));
    }
    setLoopMode(loop);
    setVolume(50); // Default volume to 50%
}

Sample::~Sample() {
    closeSample();
    if (options != 0) {
        SDL_DestroyProperties(options);
        options = 0;
    }
    if (track != nullptr) {
        MIX_DestroyTrack(track);
        track = nullptr;
    }
}

void Sample::openSample(std::filesystem::path soundPath) {
    audio = MIX_LoadAudio(mixer, soundPath.string().c_str(), true);
    if (audio == nullptr) {
        throw std::runtime_error(std::format(
            "Couldn't load {}: {}", soundPath.string(), SDL_GetError()));
    }
    MIX_SetTrackAudio(track, audio);
}

void Sample::closeSample() {
    if (track != nullptr) {
        MIX_SetTrackAudio(track, NULL);
    }
    if (audio != nullptr) {
        MIX_DestroyAudio(audio);
        audio = nullptr;
    }
}

void Sample::play() {
    if (track == nullptr) {
        std::cerr << std::format("Cannot play sound {}: track not initialized",
                                 id)
                  << std::endl;
        return;
    }
    bool play_success = MIX_PlayTrack(track, options);
    if (!play_success) {
        std::cerr << std::format("Couldn't play sound {}: {}", id,
                                 SDL_GetError())
                  << std::endl;
    }
}

void Sample::stop() {
    if (track == nullptr) {
        std::cerr << std::format("Cannot stop sound {}: track not initialized",
                                 id)
                  << std::endl;
        return;
    }
    bool stop_success = MIX_StopTrack(track, 0);
    if (!stop_success) {
        std::cerr << std::format("Couldn't stop sound {}: {}", id,
                                 SDL_GetError())
                  << std::endl;
    }
}

void Sample::setLoopMode(bool loop) {
    if (!options) {
        return;
    }
    SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, loop ? -1 : 0);
}

void Sample::setVolume(int volume) {
    if (track == nullptr) {
        std::cerr
            << std::format(
                   "Cannot set volume for sound {}: track not initialized", id)
            << std::endl;
        return;
    }
    // Convert volume so that there are more steps at lower volumes
    float gain = std::pow(static_cast<float>(volume), 2.5f) / 100000.0f;
    if (gain < 0) {
        gain = 0;
    } else if (gain > 1) {
        gain = 1;
    }
    MIX_SetTrackGain(track, gain);
}
