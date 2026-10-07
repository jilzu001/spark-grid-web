#include "audio.h"
#include "game.h"
#include "raylib.h"
#include "skins.h"
#include "ui.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static bool touchHeld[10]{}, touchPressed[10]{};
static bool photoEditing = false;
static int webNavigation = -1;
#ifdef __EMSCRIPTEN__
static Skins *browserSkins = nullptr;
static Audio *browserAudio = nullptr;
static bool browserAudioAttempted = false, browserAudioDisabled = false;
extern "C" EMSCRIPTEN_KEEPALIVE void WebAudioStart() {
    if (browserAudio && !browserAudioAttempted) {
        browserAudioAttempted = true;
        browserAudio->load(browserAudioDisabled);
    }
}
extern "C" EMSCRIPTEN_KEEPALIVE void WebAction(int action, int down) {
    if (action < 0 || action >= 10)
        return;
    if (down)
        WebAudioStart();
    if (down && !touchHeld[action])
        touchPressed[action] = true;
    touchHeld[action] = down != 0;
}
extern "C" EMSCRIPTEN_KEEPALIVE void WebPhotoEditing(int enabled) { photoEditing = enabled != 0; }
extern "C" EMSCRIPTEN_KEEPALIVE void WebNavigate(int destination) {
    if (destination >= 0 && destination <= 2) webNavigation = destination;
}
extern "C" EMSCRIPTEN_KEEPALIVE int ApplyPhotoSkin(int actor) {
    if (!browserSkins || actor < 0 || actor >= Skins::SlotCount)
        return 0;
    const std::string path = "/skins/slot-" + std::to_string(actor) + ".png";
    return browserSkins->replace(actor, path.c_str()) ? 1 : 0;
}
#endif

static void render(const Game &g, const Ui &ui, const Skins &skins, bool debug,
                   const char *notice) {
    const int t = g.config.tileSize, ox = 16;
#ifdef __EMSCRIPTEN__
    const int oy = 8;
#else
    const int oy = 40;
#endif
    ClearBackground({14, 20, 31, 255});
#ifndef __EMSCRIPTEN__
    ui.draw(ui.label(Label::Title), 16, 8, 24, {104, 226, 214, 255});
    ui.draw(TextFormat(ui.label(Label::Hud), g.activeBombs(), g.player.maxBombs, g.player.range,
                       g.aliveEnemies()),
            230, 18, 16, LIGHTGRAY);
#endif
    for (int y = 0; y < MapH; ++y)
        for (int x = 0; x < MapW; ++x) {
            int k = y * MapW + x;
            int sx = ox + x * t, sy = oy + y * t;
            DrawRectangle(sx, sy, t - 1, t - 1,
                          ((x + y) % 2) ? Color{29, 39, 51, 255} : Color{32, 43, 56, 255});
            switch (g.stage.tiles[k]) {
            case Tile::Wall:
                DrawRectangle(sx + 1, sy + 1, t - 3, t - 3, {65, 78, 96, 255});
                DrawRectangle(sx + 3, sy + 3, t - 7, 3, {92, 107, 124, 255});
                break;
            case Tile::Block:
                DrawRectangle(sx + 3, sy + 3, t - 7, t - 7, {151, 99, 58, 255});
                DrawLine(sx + 4, sy + 4, sx + t - 5, sy + t - 5, {224, 160, 86, 255});
                DrawLine(sx + t - 5, sy + 4, sx + 4, sy + t - 5, {224, 160, 86, 255});
                break;
            case Tile::Exit:
                DrawRectangleLines(sx + 4, sy + 4, t - 8, t - 8, g.aliveEnemies() ? GRAY : GREEN);
                DrawText("E", sx + 11, sy + 8, 16, g.aliveEnemies() ? GRAY : GREEN);
                break;
            case Tile::Item: {
                auto type = g.stage.items[k];
                Color c = type == ItemType::Bomb    ? SKYBLUE
                          : type == ItemType::Range ? ORANGE
                                                    : LIME;
                DrawCircle(sx + t / 2, sy + t / 2, 10, c);
                DrawText(type == ItemType::Bomb    ? "B"
                         : type == ItemType::Range ? "+"
                                                   : "S",
                         sx + 12, sy + 9, 14, {20, 30, 40, 255});
                break;
            }
            default:
                break;
            }
            if (g.fire[k] > 0) {
                DrawRectangle(sx + 2, sy + 2, t - 4, t - 4, {255, 117, 49, 230});
                DrawRectangle(sx + t / 2 - 4, sy + 1, 8, t - 2, YELLOW);
                DrawRectangle(sx + 1, sy + t / 2 - 4, t - 2, 8, YELLOW);
            }
        }
    for (const auto &b : g.bombs)
        if (b.active) {
            int x = ox + b.x * t + t / 2, y = oy + b.y * t + t / 2;
            DrawCircle(x, y, 11, {11, 15, 22, 255});
            DrawCircleLines(x, y, 11, LIGHTGRAY);
            DrawCircle(x + 4, y - 10, 3, ((int(g.time * 8) % 2) == 0) ? RED : YELLOW);
        }
    for (int i = 0; i < g.stage.enemyCount; ++i) {
        const auto &e = g.stage.enemies[i];
        if (!e.alive && e.deathTimer <= 0)
            continue;
        int x = ox + int(e.x * t), y = oy + int(e.y * t);
        if (!e.alive) {
            if (!skins.draw(3 + i * 2, x, y, 24)) {
                DrawLine(x - 8, y - 8, x + 8, y + 8, {226, 89, 130, 255});
                DrawLine(x + 8, y - 8, x - 8, y + 8, {226, 89, 130, 255});
            }
        } else if (!skins.draw(2 + i * 2, x, y, 24)) {
            DrawCircle(x, y, 10, {226, 89, 130, 255});
            DrawRectangle(x - 5, y - 3, 3, 3, BLACK);
            DrawRectangle(x + 2, y - 3, 3, 3, BLACK);
        }
    }
    int px = ox + int(g.player.x * t), py = oy + int(g.player.y * t);
    if (g.player.alive) {
        if (!skins.draw(0, px, py, 24)) {
            DrawRectangle(px - 9, py - 10, 18, 20, {93, 221, 217, 255});
            DrawRectangle(px - 6, py - 6, 12, 5, {18, 45, 62, 255});
        }
    } else {
        if (!skins.draw(1, px, py, 24)) {
            DrawLine(px - 9, py - 9, px + 9, py + 9, RED);
            DrawLine(px + 9, py - 9, px - 9, py + 9, RED);
        }
    }
#ifndef __EMSCRIPTEN__
    ui.draw(ui.label(Label::Footer), 16, 461, ui.korean ? 14 : 12, GRAY);
#endif
    if (notice[0])
        ui.draw(notice, 24, 436, 12, ORANGE);
    if (debug) {
        DrawRectangle(20, 54, 305, 104, {0, 0, 0, 220});
        ui.draw(
            TextFormat(ui.label(Label::DebugActors), GetFPS(), g.activeBombs(), g.aliveEnemies()),
            28, 62, 16, GREEN);
        ui.draw(TextFormat(ui.label(Label::DebugPosition), g.player.x, g.player.y, g.player.range),
                28, 86, 16, GREEN);
        ui.draw(TextFormat(ui.label(Label::DebugSpeed), g.player.speed, g.time), 28, 110, 16,
                GREEN);
        ui.draw(ui.label(Label::ExitHint), 28, 134, 14, LIGHTGRAY);
        DrawRectangle(332, 54, 288, 24 + g.stage.enemyCount * 19, {0, 0, 0, 220});
        for (int i = 0; i < g.stage.enemyCount; ++i) {
            const auto &e = g.stage.enemies[i];
            if (!e.alive)
                continue;
            Label state = e.blocked  ? Label::EnemyBlocked
                          : e.moving ? Label::EnemyMoving
                                     : Label::EnemyChoosing;
            ui.draw(TextFormat(ui.label(Label::DebugEnemy), i + 1, int(e.x), int(e.y), e.targetX,
                               e.targetY, ui.label(state)),
                    340, 62 + i * 19, 12, GREEN);
        }
    }
    if (g.phase != Phase::Playing) {
        DrawRectangle(20, 188, GetScreenWidth()-40, 112, {11, 16, 24, 240});
        ui.center(ui.label(g.phase == Phase::Dead ? Label::Dead : Label::Won), 207, 30,
                  g.phase == Phase::Dead ? Color{255, 130, 120, 255} : LIME);
        ui.center(ui.label(Label::Restart), 258, 18, WHITE);
    }
}
enum class Screen { Title, Help, Playing };
static void renderMenu(const Ui &ui, const Audio &audio, Screen screen, int selected) {
    ClearBackground({14, 20, 31, 255});
    for (int y = 0; y < 15; ++y)
        for (int x = 0; x < 20; ++x)
            DrawRectangle(x * 32 + 1, y * 32 + 1, 30, 30,
                          ((x + y) % 2) ? Color{18, 27, 40, 255} : Color{20, 30, 43, 255});
    if (screen == Screen::Help) {
        ui.center(ui.label(Label::Help), 70, 34, {104, 226, 214, 255});
        ui.center(ui.label(Label::Controls), 160, 21, WHITE);
        ui.center(ui.label(Label::Goal), 205, 20, LIGHTGRAY);
        ui.center(ui.label(Label::Rules), 245, 18, LIGHTGRAY);
        ui.center(ui.label(Label::Items), 285, 18, {240, 186, 101, 255});
        ui.center(ui.label(Label::Restart), 326, 18, GRAY);
        DrawRectangleRounded({180, 375, 280, 45}, .2f, 8, {40, 70, 84, 255});
        ui.center(ui.label(Label::Back), 385, 23, WHITE);
    } else {
        int pulse = int(3 * std::sin(GetTime() * 3));
        DrawCircle(320, 69, 23 + pulse, {28, 70, 81, 255});
        DrawCircle(320, 69, 17, {96, 222, 211, 255});
        DrawCircle(320, 69, 12, {14, 20, 31, 255});
        DrawRectangle(332, 44, 5, 13, {241, 181, 89, 255});
        ui.center(ui.label(Label::Title), 114, 48, {104, 226, 214, 255});
        ui.center(ui.label(Label::Subtitle), 177, 20, LIGHTGRAY);
        for (int i = 0; i < 3; ++i) {
            bool active = i == selected;
            DrawRectangleRounded({180, float(235 + i * 54), 280, 44}, .2f, 8,
                                 active ? Color{40, 91, 100, 255} : Color{26, 39, 54, 255});
            ui.center(ui.label(i == 0   ? Label::Start
                               : i == 1 ? Label::Help
                                        : Label::Quit),
                      244 + i * 54, 24, active ? WHITE : GRAY);
        }
        ui.center(ui.label(Label::MenuHint), 423, 16, GRAY);
    }
    ui.draw(ui.label(!audio.ready  ? Label::AudioUnavailable
                     : audio.muted ? Label::AudioOff
                                   : Label::AudioOn),
            18, 454, 15, {140, 163, 177, 255});
}
struct App {
    const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    double firstFrameMs = 0;
    bool smoke = false, noAudio = false, english = false, debug = false, quit = false;
    int frames = 0, selected = 0;
    unsigned checks = 0;
    std::string error;
    Game g;
    Ui ui;
    Audio audio;
    Skins skins;
    Screen screen = Screen::Title;
    bool initialize() {
        Stage stage;
#ifdef __EMSCRIPTEN__
        const std::string base = "/";
#else
        const std::string base = GetApplicationDirectory();
#endif
        if (!loadStage(base + "data/stage01.txt", stage, error))
            stage = defaultStage();
        g = Game(stage);
        SetTraceLogLevel(LOG_WARNING);
#ifdef __EMSCRIPTEN__
        InitWindow(640, 432, "Spark Grid - native action puzzle");
#else
        InitWindow(640, 480, "Spark Grid - native action puzzle");
#endif
        if (!IsWindowReady())
            return false;
        SetExitKey(KEY_NULL);
        ui.load(english);
        skins.load();
#ifdef __EMSCRIPTEN__
        browserSkins = &skins;
        browserAudio = &audio;
        browserAudioDisabled = noAudio;
#else
        audio.load(noAudio);
        SetTargetFPS(60);
#endif
        return true;
    }
    void frame() {
        if (quit) return;
#ifdef __EMSCRIPTEN__
        const Screen previousScreen = screen;
        if (webNavigation >= 0) {
            if (webNavigation > 0) {
                Stage source = defaultStage();
                if (webNavigation == 2) {
                    source.tiles.fill(Tile::Wall);
                    for (int y=1;y<12;++y) for(int x=1;x<10;++x) {
                        auto &tile=source.tiles[y*MapW+x];
                        tile=(x%2==0&&y%2==0)?Tile::Wall:Tile::Floor;
                        if(tile==Tile::Floor&&x+y>4&&x!=9&&(x*7+y*3)%5<2)tile=Tile::Block;
                    }
                    source.tiles[11*MapW+9]=Tile::Exit;
                    for(int i=0;i<3;++i){source.enemies[i].x=9.5f;source.enemies[i].y=3.5f+i*3;}
                }
                g=Game(source,g.config);
                SetWindowSize(webNavigation==2?384:640,432);
                screen = Screen::Playing;
            }
            else screen = Screen::Title;
            photoEditing = false;
            webNavigation = -1;
            audio.play(Cue::Menu);
        }
#endif
#ifndef __EMSCRIPTEN__
        // raylib 5.5's web WindowShouldClose sleeps for synchronous loops.
        // Our web callback already yields to the browser between frames.
        if (WindowShouldClose()) {
            quit = true;
            return;
        }
#endif
        bool up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || touchPressed[2];
        bool down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || touchPressed[3];
        bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || touchPressed[4] ||
                       touchPressed[5];
        bool back = IsKeyPressed(KEY_ESCAPE) || touchPressed[7],
             restart = IsKeyPressed(KEY_R) || touchPressed[6];
        bool mute = IsKeyPressed(KEY_M) || touchPressed[8],
             toggleDebug = IsKeyPressed(KEY_F1) || touchPressed[9];
        Input input{int(IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D) || touchHeld[1]) -
                        int(IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A) || touchHeld[0]),
                    int(IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S) || touchHeld[3]) -
                        int(IsKeyDown(KEY_UP) || IsKeyDown(KEY_W) || touchHeld[2]),
                    IsKeyPressed(KEY_SPACE) || touchPressed[4]};
        const int pressedDx = int(IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || touchPressed[1]) -
                              int(IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || touchPressed[0]);
        const int pressedDy = int(IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || touchPressed[3]) -
                              int(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || touchPressed[2]);
        if (pressedDx || pressedDy) {
            input.dx = pressedDx;
            input.dy = pressedDy;
            input.movePressed = true;
        }
        // Scripted checks use the same screen handlers as real keyboard input.
        if (smoke) {
            up = frames == 18;
            down = frames == 5;
            confirm = frames == 6 || frames == 19 || frames == 190;
            back = frames == 12 || frames == 170;
            restart = frames == 160;
            mute = frames == 180 || frames == 185;
            toggleDebug = frames == 192;
            input = {0, 0, frames == 21};
        }
        if (mute)
            audio.toggle();
        if (screen == Screen::Title) {
            if (up || down) {
                selected = (selected + (down ? 1 : 2)) % 3;
                audio.play(Cue::Menu);
            }
            Vector2 mouse = GetMousePosition();
            for (int i = 0; i < 3; ++i)
                if (!smoke && CheckCollisionPointRec(mouse, {180, float(235 + i * 54), 280, 44}) &&
                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    selected = i;
                    confirm = true;
                }
            if (back)
                quit = true;
            if (confirm) {
                audio.play(Cue::Menu);
                if (selected == 0) {
                    g.restart();
                    screen = Screen::Playing;
                } else if (selected == 1)
                    screen = Screen::Help;
                else
                    quit = true;
            }
        } else if (screen == Screen::Help) {
            if (back || confirm ||
                (!smoke && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                 CheckCollisionPointRec(GetMousePosition(), {180, 375, 280, 45}))) {
                screen = Screen::Title;
                audio.play(Cue::Menu);
            }
        } else {
            if (back) {
                screen = Screen::Title;
                audio.play(Cue::Menu);
            } else {
                if (restart) {
                    g.restart();
                    audio.play(Cue::Menu);
                }
                if (toggleDebug)
                    debug = !debug;
                // A menu's Space confirmation must not also place a bomb.
                if (!photoEditing) {
                    g.update(smoke ? 1.f / 60 : GetFrameTime(), input);
                    audio.events(g.events);
                }
            }
        }
#ifdef __EMSCRIPTEN__
        if (screen != previousScreen)
            EM_ASM({ if (Module.onWebScreen) Module.onWebScreen($0); },
                   screen == Screen::Playing ? 1 : 0);
        if (screen == Screen::Playing && frames % 10 == 0)
            EM_ASM({ if(Module.onHud)Module.onHud($0,$1,$2,$3,$4); },
                   g.activeBombs(),g.player.maxBombs,g.player.range,g.aliveEnemies(),audio.muted);
#endif
        audio.music(screen != Screen::Playing   ? BackgroundMusic::Title
                    : g.phase == Phase::Playing ? BackgroundMusic::Gameplay
                                                : BackgroundMusic::None);
        BeginDrawing();
        if (screen == Screen::Playing)
            render(g, ui, skins, debug, error.empty() ? "" : ui.label(Label::StageError));
        else
            renderMenu(ui, audio, screen, selected);
        EndDrawing();
        if (frames == 0)
            firstFrameMs = std::chrono::duration<double, std::milli>(
                               std::chrono::steady_clock::now() - started)
                               .count();
        ++frames;
        if (smoke) {
            if (frames == 3) {
                TakeScreenshot("title.png");
                if (screen == Screen::Title && g.time == 0)
                    checks |= 1;
            }
            if (frames == 10) {
                TakeScreenshot("help.png");
                if (screen == Screen::Help)
                    checks |= 2;
            }
            if (frames == 22 && g.activeBombs() == 1)
                checks |= 4;
            if (frames == 150) {
                TakeScreenshot("death.png");
                if (g.phase == Phase::Dead)
                    checks |= 8;
            }
            if (frames == 161 && g.phase == Phase::Playing && g.activeBombs() == 0)
                checks |= 16;
            if (frames == 171 && screen == Screen::Title)
                checks |= 32;
            if (frames == 181 && audio.muted)
                checks |= 64;
            if (frames == 186 && !audio.muted)
                checks |= 128;
        }
        if (smoke && frames >= 240) {
            TakeScreenshot("smoke.png");
            std::printf("SMOKE %s: %d frames, first frame %.1f ms, stage %s, language %s, audio "
                        "%s, plays %u, checks %u/255\n",
                        checks == 255 ? "PASS" : "FAIL", frames, firstFrameMs,
                        error.empty() ? "loaded" : "fallback", ui.korean ? "Korean" : "English",
                        audio.ready ? "ready" : "disabled", audio.played, checks);
            quit = true;
        }

        for (auto &pressed : touchPressed)
            pressed = false;
    }
    void shutdown() {
        audio.unload();
        skins.unload();
        ui.unload();
        CloseWindow();
    }
};
int main(int argc, char **argv) {
    App *app = new App;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--smoke") == 0)
            app->smoke = true;
        if (std::strcmp(argv[i], "--no-audio") == 0)
            app->noAudio = true;
        if (std::strcmp(argv[i], "--english") == 0)
            app->english = true;
    }
    if (!app->initialize()) {
        delete app;
        return 1;
    }
#ifdef __EMSCRIPTEN__
    // Persistent state lives on the heap; browser invokes one frame at a time.
    emscripten_set_main_loop_arg(
        [](void *context) {
            auto *running = static_cast<App *>(context);
            running->frame();
            if (running->quit) {
                emscripten_cancel_main_loop();
                running->shutdown();
                browserSkins = nullptr;
                browserAudio = nullptr;
                EM_ASM({
                    if (Module.onGameClosed)
                        Module.onGameClosed();
                });
                delete running;
            }
        },
        app, 0, true);
    return 0;
#else
    while (!app->quit)
        app->frame();
    const int result = app->smoke && app->checks != 255 ? 2 : 0;
    app->shutdown();
    delete app;
    return result;
#endif
}

