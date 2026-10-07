#pragma once
#include "game.h"
#include "raylib.h"
#include <array>
enum class Cue { Menu, Place, Blast, Pickup, Death, Win, Theme, Gameplay, Count };
enum class BackgroundMusic { None, Title, Gameplay };
struct Audio {
    std::array<Sound, int(Cue::Count)> sounds{};
    bool ready = false, muted = false;
    unsigned played = 0;
    BackgroundMusic currentMusic = BackgroundMusic::None;
    void load(bool disabled = false);
    void unload();
    void play(Cue cue);
    void toggle();
    void music(BackgroundMusic mode);
    void events(const GameEvents &events);
};
