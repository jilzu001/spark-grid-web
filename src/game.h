#pragma once
#include <array>
#include <cstdint>
#include <string>

constexpr int MapW = 19, MapH = 13, Cells = MapW * MapH;
enum class Tile : int { Floor, Wall, Block, Exit, Item };
enum class ItemType { None, Bomb, Range, Speed };
enum class Phase { Playing, Dead, Won };
struct GameConfig {
    int tileSize = 32, initialBombs = 1, initialRange = 2;
    float playerSpeed = 3.4f, bombTimer = 2.0f, explosionDuration = .45f;
    float enemySpeed = 1.5f, itemDropRate = .3f;
};
struct Actor {
    float x = 1.5f, y = 1.5f;
    bool alive = true;
};
struct Player : Actor {
    int maxBombs = 1, range = 2;
    float speed = 3.4f;
};
struct Bomb {
    int x = 0, y = 0, range = 2;
    float placedAt = 0, fuse = 2;
    bool active = false, passOwner = false;
};
struct Enemy : Actor {
    int dx = 0, dy = 0;
    int targetX = 0, targetY = 0, fromX = 0, fromY = 0;
    bool moving = false, blocked = false, returning = false;
    float deathTimer = 0;
};
struct Input {
    int dx = 0, dy = 0;
    bool bomb = false;
};
// One update's audio notifications; game rules still have no audio dependency.
struct GameEvents {
    int placed = 0, exploded = 0, collected = 0;
    bool died = false, won = false;
};
struct Stage {
    std::array<Tile, Cells> tiles{};
    std::array<ItemType, Cells> items{};
    std::array<Enemy, 8> enemies{};
    int enemyCount = 0;
    float spawnX = 1.5f, spawnY = 1.5f;
};
Stage defaultStage();
bool loadStage(const std::string &path, Stage &result, std::string &error);
struct Game {
    GameConfig config{};
    Stage stage{};
    Player player{};
    std::array<Bomb, 32> bombs{};
    std::array<float, Cells> fire{};
    Phase phase = Phase::Playing;
    GameEvents events{};
    float time = 0;
    uint32_t randomState = 1234567;
    explicit Game(const Stage &source = defaultStage(), GameConfig cfg = {});
    void restart();
    void update(float dt, Input input);
    bool placeBomb();
    void explode(int index);
    bool canStand(float x, float y, bool owner) const;
    void move(Actor &actor, float dx, float dy, bool owner);
    int activeBombs() const;
    int aliveEnemies() const;
    uint32_t random();
    void damageAndCollect();

  private:
    Stage original{};
    void moveEnemy(Enemy &enemy, float dt);
    void movePlayer(int dx, int dy, float dt);
};
