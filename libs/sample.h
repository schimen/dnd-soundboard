#pragma once
#include <SDL3/SDL_properties.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <filesystem>

class Sample {
  public:
    Sample(const int id, MIX_Mixer *mixer);
    ~Sample();
    void openSample(std::filesystem::path soundPath);
    void closeSample();
    void play();
    void stop();
    void setLoopMode(bool loop);
    void setVolume(int volume);
    auto getId() const { return id; }

  private:
    int id;
    bool loop = false;
    MIX_Mixer *mixer = nullptr;
    MIX_Audio *audio = nullptr;
    MIX_Track *track = nullptr;
    SDL_PropertiesID options = 0;
};
