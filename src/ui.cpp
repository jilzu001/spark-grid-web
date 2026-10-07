#include "ui.h"
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

static const char *Korean[] = {
    u8"스파크 그리드",
    u8"폭탄으로 길을 열고, 출구를 찾아라",
    u8"게임 시작",
    u8"플레이 방법",
    u8"게임 종료",
    u8"돌아가기",
    u8"이동: 방향키 / WASD   폭탄: Space",
    u8"적을 모두 제거한 뒤 초록색 출구로 이동하세요.",
    u8"폭탄은 2초 뒤 폭발합니다. 자신도 맞으면 사망합니다.",
    u8"아이템: B 폭탄 추가 / + 범위 증가 / S 속도 증가",
    u8"방향키로 선택 · Enter 확인 · M 소리 켜기/끄기",
    u8"이동 WASD/방향키  폭탄 Space  재시작 R  디버그 F1  소리 M  메뉴 Esc",
    u8"폭탄 %d/%d   범위 %d   적 %d",
    u8"FPS %d   활성 폭탄 %d   적 %d",
    u8"위치 %.2f, %.2f   폭발 범위 %d",
    u8"속도 %.1f칸/초   시간 %.1f초",
    u8"적을 모두 제거하면 출구가 열립니다",
    u8"신호가 끊겼습니다",
    u8"구역 돌파 성공!",
    u8"R 다시 도전  ·  Esc 타이틀로",
    u8"맵 파일 오류: 내장 맵을 사용합니다",
    u8"소리 켜짐 [M]",
    u8"소리 꺼짐 [M]",
    u8"소리 장치 없음",
    u8"적 %d  %d,%d > %d,%d  %s",
    u8"이동",
    u8"막힘",
    u8"선택"};
static const char *English[] = {
    "SPARK GRID",
    "Open a path. Clear the sector.",
    "START GAME",
    "HOW TO PLAY",
    "QUIT",
    "BACK",
    "Move: arrows / WASD   Bomb: Space",
    "Clear every enemy, then reach the green exit.",
    "Bombs explode after 2 seconds. Keep out of the blast.",
    "Items: B more bombs / + range / S speed",
    "Arrows select / Enter confirm / M sound",
    "WASD/Arrows move  Space bomb  R retry  F1 debug  M sound  Esc menu",
    "BOMBS %d/%d   RANGE %d   ENEMIES %d",
    "FPS %d   Bombs %d   Enemies %d",
    "Position %.2f, %.2f   Range %d",
    "Speed %.1f tiles/s   Time %.1fs",
    "Clear all enemies to open the exit",
    "SIGNAL LOST",
    "SECTOR CLEAR!",
    "R retry / Esc title",
    "Stage file invalid: using built-in map",
    "Sound ON [M]",
    "Sound OFF [M]",
    "Audio unavailable",
    "Enemy %d  %d,%d > %d,%d  %s",
    "Moving",
    "Blocked",
    "Choosing"};
static_assert(sizeof(Korean) / sizeof(*Korean) == int(Label::Count));
static_assert(sizeof(English) / sizeof(*English) == int(Label::Count));

void Ui::load(bool forceEnglish) {
    font = GetFontDefault();
    if (forceEnglish)
        return;
    std::vector<int> glyphs;
    for (int c = 32; c < 127; ++c)
        glyphs.push_back(c);
    for (const char *text : Korean) {
        int count = 0;
        int *points = LoadCodepoints(text, &count);
        for (int i = 0; i < count; ++i)
            if (std::find(glyphs.begin(), glyphs.end(), points[i]) == glyphs.end())
                glyphs.push_back(points[i]);
        UnloadCodepoints(points);
    }
    // Do not redistribute Windows fonts. Read installed Malgun Gothic once;
    // a user-supplied data/ui.ttf takes priority.
#ifdef __EMSCRIPTEN__
    std::string path = "/data/ui.ttf";
#else
    std::string path = std::string(GetApplicationDirectory()) + "data/ui.ttf";
#endif
    if (!FileExists(path.c_str())) {
        const char *windows = std::getenv("WINDIR");
        path = std::string(windows ? windows : "C:/Windows") + "/Fonts/malgun.ttf";
    }
    if (!FileExists(path.c_str()))
        return;
    Font loaded = LoadFontEx(path.c_str(), 36, glyphs.data(), int(glyphs.size()));
    if (IsFontValid(loaded) && loaded.texture.id != font.texture.id) {
        font = loaded;
        korean = true;
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    }
}
void Ui::unload() {
    if (korean)
        UnloadFont(font);
    korean = false;
}
const char *Ui::label(Label id) const { return korean ? Korean[int(id)] : English[int(id)]; }
void Ui::draw(const char *text, int x, int y, int size, Color color) const {
    DrawTextEx(font, text, {float(x), float(y)}, float(size), korean ? 0.5f : 1.f, color);
}
void Ui::center(const char *text, int y, int size, Color color) const {
    int x = int((GetScreenWidth() - MeasureTextEx(font, text, float(size), korean ? 0.5f : 1.f).x) / 2);
    draw(text, x, y, size, color);
}
