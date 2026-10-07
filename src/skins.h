#pragma once
#include "raylib.h"
#include <array>
struct Skins {
    // Slots: player normal/dead, then 8 enemy normal/dead pairs.
    static constexpr int MaxEnemySets = 8, SlotCount = 2 + MaxEnemySets * 2;
    std::array<Texture2D, SlotCount> textures{};
    void load();
    bool replace(int actor, const char *path);
    bool draw(int actor, int x, int y, int size) const;
    void unload();
};
