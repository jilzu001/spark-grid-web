#include "game.h"
#include <algorithm>
#include <cmath>
#include <fstream>

static int cell(float x, float y) { return int(y) * MapW + int(x); }
Stage defaultStage() {
    Stage s;
    for (int y = 0; y < MapH; ++y)
        for (int x = 0; x < MapW; ++x) {
            s.tiles[y * MapW + x] =
                (x == 0 || y == 0 || x == MapW - 1 || y == MapH - 1 || (x % 2 == 0 && y % 2 == 0))
                    ? Tile::Wall
                    : Tile::Floor;
            if (s.tiles[y * MapW + x] == Tile::Floor && x + y > 4 && (x * 7 + y * 3) % 5 < 2)
                s.tiles[y * MapW + x] = Tile::Block;
        }
    s.tiles[11 * MapW + 17] = Tile::Exit;
    s.enemyCount = 3;
    for (int i = 0; i < 3; ++i) {
        s.enemies[i].x = 15.5f;
        s.enemies[i].y = 3.5f + i * 4;
        s.tiles[cell(s.enemies[i].x, s.enemies[i].y)] = Tile::Floor;
    }
    return s;
}
bool loadStage(const std::string &path, Stage &result, std::string &error) {
    std::ifstream f(path);
    Stage s;
    int w, h;
    if (!(f >> w >> h) || w != MapW || h != MapH) {
        error = "Expected 19 13 stage header";
        return false;
    }
    for (auto &t : s.tiles) {
        int n;
        if (!(f >> n) || n < 0 || n > 4) {
            error = "Invalid tile data";
            return false;
        }
        t = Tile(n);
    }
    int px, py;
    if (!(f >> px >> py >> s.enemyCount) || s.enemyCount < 0 || s.enemyCount > 8) {
        error = "Invalid spawn data";
        return false;
    }
    auto valid = [&](int x, int y) {
        return x > 0 && x < MapW - 1 && y > 0 && y < MapH - 1 &&
               (s.tiles[y * MapW + x] == Tile::Floor || s.tiles[y * MapW + x] == Tile::Exit ||
                s.tiles[y * MapW + x] == Tile::Item);
    };
    if (!valid(px, py)) {
        error = "Invalid player spawn";
        return false;
    }
    s.spawnX = px + .5f;
    s.spawnY = py + .5f;
    for (int i = 0; i < s.enemyCount; ++i) {
        int x, y;
        if (!(f >> x >> y) || !valid(x, y)) {
            error = "Invalid enemy spawn";
            return false;
        }
        s.enemies[i].x = x + .5f;
        s.enemies[i].y = y + .5f;
    }
    for (int y = 0; y < MapH; ++y)
        for (int x = 0; x < MapW; ++x) {
            int k = y * MapW + x;
            if ((x == 0 || y == 0 || x == MapW - 1 || y == MapH - 1) && s.tiles[k] != Tile::Wall) {
                error = "Border must be walls";
                return false;
            }
            if (s.tiles[k] == Tile::Item)
                s.items[k] = ItemType::Bomb;
        }
    result = s;
    error.clear();
    return true;
}
Game::Game(const Stage &source, GameConfig cfg) : config(cfg), original(source) { restart(); }
void Game::restart() {
    stage = original;
    for (int i = 0; i < stage.enemyCount; ++i) {
        auto &e = stage.enemies[i];
        e.targetX = e.fromX = int(e.x);
        e.targetY = e.fromY = int(e.y);
        e.moving = e.blocked = e.returning = false;
        e.dx = e.dy = 0;
        e.deathTimer = 0;
    }
    player = Player{};
    player.x = stage.spawnX;
    player.y = stage.spawnY;
    player.speed = config.playerSpeed;
    player.maxBombs = config.initialBombs;
    player.range = config.initialRange;
    bombs = {};
    fire = {};
    phase = Phase::Playing;
    time = 0;
    randomState = 1234567;
    events = {};
}
uint32_t Game::random() {
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return randomState;
}
int Game::activeBombs() const {
    int n = 0;
    for (const auto &b : bombs)
        n += b.active;
    return n;
}
int Game::aliveEnemies() const {
    int n = 0;
    for (int i = 0; i < stage.enemyCount; ++i)
        n += stage.enemies[i].alive;
    return n;
}
bool Game::canStand(float x, float y, bool owner) const {
    constexpr float r = .28f;
    if (x - r < 0 || y - r < 0 || x + r >= MapW || y + r >= MapH)
        return false;
    for (int cy = int(y - r); cy <= int(y + r); ++cy)
        for (int cx = int(x - r); cx <= int(x + r); ++cx) {
            auto t = stage.tiles[cy * MapW + cx];
            if (t == Tile::Wall || t == Tile::Block)
                return false;
            for (const auto &b : bombs)
                if (b.active && b.x == cx && b.y == cy && !(owner && b.passOwner))
                    return false;
        }
    return true;
}
void Game::move(Actor &a, float dx, float dy, bool owner) {
    if (canStand(a.x + dx, a.y, owner))
        a.x += dx;
    if (canStand(a.x, a.y + dy, owner))
        a.y += dy;
}
bool Game::placeBomb() {
    if (phase != Phase::Playing || activeBombs() >= player.maxBombs)
        return false;
    int x = int(player.x), y = int(player.y);
    for (auto &b : bombs)
        if (b.active && b.x == x && b.y == y)
            return false;
    for (auto &b : bombs)
        if (!b.active) {
            b = {x, y, player.range, time, config.bombTimer, true, true};
            ++events.placed;
            return true;
        }
    return false;
}
void Game::explode(int index) {
    if (!bombs[index].active)
        return;
    const Bomb b = bombs[index];
    bombs[index].active = false;
    ++events.exploded;
    auto ignite = [&](int x, int y) {
        int k = y * MapW + x;
        fire[k] = config.explosionDuration;
        if (stage.tiles[k] == Tile::Item) {
            stage.tiles[k] = Tile::Floor;
            stage.items[k] = ItemType::None;
        }
        for (int i = 0; i < int(bombs.size()); ++i)
            if (bombs[i].active && bombs[i].x == x && bombs[i].y == y)
                explode(i);
    };
    ignite(b.x, b.y);
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto &d : dirs)
        for (int r = 1; r <= b.range; ++r) {
            int x = b.x + d[0] * r, y = b.y + d[1] * r;
            if (x < 0 || y < 0 || x >= MapW || y >= MapH)
                break;
            int k = y * MapW + x;
            auto t = stage.tiles[k];
            if (t == Tile::Wall)
                break;
            if (t == Tile::Block) {
                fire[k] = config.explosionDuration;
                stage.tiles[k] = Tile::Floor;
                if ((random() % 10000) / 10000.f < config.itemDropRate) {
                    stage.tiles[k] = Tile::Item;
                    stage.items[k] = ItemType(1 + random() % 3);
                }
                break;
            }
            ignite(x, y);
        }
}
void Game::damageAndCollect() {
    for (int i = 0; i < stage.enemyCount; ++i) {
        auto &e = stage.enemies[i];
        if (e.alive && fire[cell(e.x, e.y)] > 0) {
            e.alive = false;
            e.deathTimer = .7f;
        }
    }
    bool hit = fire[cell(player.x, player.y)] > 0;
    for (int i = 0; i < stage.enemyCount; ++i) {
        auto &e = stage.enemies[i];
        if (e.alive && std::abs(e.x - player.x) < .5f && std::abs(e.y - player.y) < .5f)
            hit = true;
    }
    if (hit) {
        player.alive = false;
        phase = Phase::Dead;
        events.died = true;
        return;
    }
    int k = cell(player.x, player.y);
    if (stage.tiles[k] == Tile::Item) {
        ++events.collected;
        switch (stage.items[k]) {
        case ItemType::Bomb:
            player.maxBombs = std::min(32, player.maxBombs + 1);
            break;
        case ItemType::Range:
            player.range = std::min(MapW, player.range + 1);
            break;
        case ItemType::Speed:
            player.speed = std::min(6.f, player.speed + .4f);
            break;
        default:
            break;
        }
        stage.tiles[k] = Tile::Floor;
        stage.items[k] = ItemType::None;
    }
    if (stage.tiles[k] == Tile::Exit && aliveEnemies() == 0) {
        phase = Phase::Won;
        events.won = true;
    }
}
void Game::moveEnemy(Enemy &e, float dt) {
    float budget = std::max(0.f, config.enemySpeed) * dt;
    e.blocked = false;
    while (budget > 0) {
        if (!e.moving) {
            const int x = int(e.x), y = int(e.y);
            const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            int choices[4], count = 0, reverse = -1;
            for (int d = 0; d < 4; ++d) {
                if (!canStand(x + dirs[d][0] + .5f, y + dirs[d][1] + .5f, false))
                    continue;
                if (dirs[d][0] == -e.dx && dirs[d][1] == -e.dy)
                    reverse = d;
                else
                    choices[count++] = d;
            }
            // Reverse only at a dead end. Select from legal neighbours, never walls.
            if (count == 0 && reverse >= 0)
                choices[count++] = reverse;
            if (count == 0) {
                e.targetX = x;
                e.targetY = y;
                e.blocked = true;
                break;
            }
            const int d = choices[random() % count];
            e.dx = dirs[d][0];
            e.dy = dirs[d][1];
            e.fromX = x;
            e.fromY = y;
            e.targetX = x + e.dx;
            e.targetY = y + e.dy;
            e.moving = true;
            e.returning = false;
        }
        // A newly placed bomb may close the destination during movement.
        // Return along the same segment rather than turning between centres.
        if (!canStand(e.targetX + .5f, e.targetY + .5f, false)) {
            if (!e.returning) {
                e.targetX = e.fromX;
                e.targetY = e.fromY;
                e.dx = -e.dx;
                e.dy = -e.dy;
                e.returning = true;
            }
            if (!canStand(e.targetX + .5f, e.targetY + .5f, false)) {
                e.blocked = true;
                break;
            }
        }
        const float tx = e.targetX + .5f, ty = e.targetY + .5f;
        const float distance = std::abs(tx - e.x) + std::abs(ty - e.y);
        const float travel = std::min(budget, distance);
        const float nx = e.x + (tx > e.x ? travel : tx < e.x ? -travel : 0);
        const float ny = e.y + (ty > e.y ? travel : ty < e.y ? -travel : 0);
        if (!canStand(nx, ny, false)) {
            e.blocked = true;
            break;
        }
        e.x = nx;
        e.y = ny;
        budget -= travel;
        // An enemy cannot cross a burning tile to escape within this substep.
        if (fire[cell(e.x, e.y)] > 0) {
            e.alive = false;
            e.deathTimer = .7f;
            break;
        }
        if (travel >= distance) {
            e.x = tx;
            e.y = ty;
            e.moving = false;
        }
    }
}
void Game::movePlayer(int dx, int dy, float dt) {
    const float distance = player.speed * dt;
    const float nx = player.x + dx * distance, ny = player.y + dy * distance;
    if (canStand(nx, ny, true)) {
        player.x = nx;
        player.y = ny;
        return;
    }
    const float cx = int(player.x) + .5f, cy = int(player.y) + .5f;
    const float offset = dx != 0 ? cy - player.y : cx - player.x;
    if ((dx == 0 && dy == 0) || std::abs(offset) > .35f || !canStand(cx + dx, cy + dy, true))
        return;
    const float correction = std::clamp(offset, -distance, distance);
    move(player, dx != 0 ? 0 : correction, dx != 0 ? correction : 0, true);
}
void Game::update(float dt, Input input) {
    events = {};
    if (phase != Phase::Playing)
        return;
    // Substeps preserve collision and timer behavior across long frames.
    if (input.bomb)
        placeBomb();
    dt = std::clamp(dt, 0.f, .25f);
    while (dt > 0) {
        float step = std::min(dt, 1.f / 120);
        dt -= step;
        time += step;
        for (auto &f : fire)
            f = std::max(0.f, f - step);
        int dx = input.dx, dy = input.dy;
        if (dx != 0)
            dy = 0;
        movePlayer(dx, dy, step);
        for (auto &b : bombs)
            if (b.active && b.passOwner &&
                (std::abs(player.x - (b.x + .5f)) > .78f ||
                 std::abs(player.y - (b.y + .5f)) > .78f))
                b.passOwner = false;
        for (int i = 0; i < int(bombs.size()); ++i)
            if (bombs[i].active && (time - bombs[i].placedAt >= bombs[i].fuse ||
                                    fire[bombs[i].y * MapW + bombs[i].x] > 0))
                explode(i);
        for (int i = 0; i < stage.enemyCount; ++i) {
            auto &e = stage.enemies[i];
            if (!e.alive) {
                e.deathTimer = std::max(0.f, e.deathTimer - step);
                continue;
            }
            if (fire[cell(e.x, e.y)] > 0) {
                e.alive = false;
                e.deathTimer = .7f;
                continue;
            }
            moveEnemy(e, step);
        }
        damageAndCollect();
        if (phase != Phase::Playing)
            break;
    }
}

