#pragma once
#include "raylib.h"
enum class Label {
    Title,
    Subtitle,
    Start,
    Help,
    Quit,
    Back,
    Controls,
    Goal,
    Rules,
    Items,
    MenuHint,
    Footer,
    Hud,
    DebugActors,
    DebugPosition,
    DebugSpeed,
    ExitHint,
    Dead,
    Won,
    Restart,
    StageError,
    AudioOn,
    AudioOff,
    AudioUnavailable,
    DebugEnemy,
    EnemyMoving,
    EnemyBlocked,
    EnemyChoosing,
    Count
};
struct Ui {
    Font font{};
    bool korean = false;
    void load(bool forceEnglish = false);
    void unload();
    const char *label(Label id) const;
    void draw(const char *text, int x, int y, int size, Color color) const;
    void center(const char *text, int y, int size, Color color) const;
};
