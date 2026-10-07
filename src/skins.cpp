#include "skins.h"
#include <string>
void Skins::load() {
#ifdef __EMSCRIPTEN__
    const std::string base = "/";
#else
    const std::string base = GetApplicationDirectory();
#endif
    replace(0, (base + "data/player.png").c_str());
    replace(1, (base + "data/player-dead.png").c_str());
    for (int i = 0; i < MaxEnemySets; ++i) {
        replace(2 + i * 2, (base + "data/enemy-" + std::to_string(i + 1) + ".png").c_str());
        replace(3 + i * 2, (base + "data/enemy-" + std::to_string(i + 1) + "-dead.png").c_str());
    }
}
bool Skins::replace(int actor, const char *path) {
    if (actor < 0 || actor >= int(textures.size()) || !FileExists(path))
        return false;
    Image image = LoadImage(path);
    if (!IsImageValid(image))
        return false;
    // Browser bridge supplies 32x32 PNG. Keep the same size for optional desktop skins.
    ImageResizeNN(&image, 32, 32);
    Texture2D replacement = LoadTextureFromImage(image);
    UnloadImage(image);
    if (!IsTextureValid(replacement))
        return false;
    SetTextureFilter(replacement, TEXTURE_FILTER_POINT);
    if (IsTextureValid(textures[actor]))
        UnloadTexture(textures[actor]);
    textures[actor] = replacement;
    return true;
}
bool Skins::draw(int actor, int x, int y, int size) const {
    if (actor < 0 || actor >= int(textures.size()))
        return false;
    if (!IsTextureValid(textures[actor]) && actor >= 2)
        actor = 2 + actor % 2;
    if (!IsTextureValid(textures[actor]))
        return false;
    auto t = textures[actor];
    DrawTexturePro(t, {0, 0, float(t.width), float(t.height)},
                   {float(x - size / 2), float(y - size / 2), float(size), float(size)}, {0, 0}, 0,
                   WHITE);
    return true;
}
void Skins::unload() {
    for (auto &t : textures) {
        if (IsTextureValid(t))
            UnloadTexture(t);
        t = {};
    }
}
