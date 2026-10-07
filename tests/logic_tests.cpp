#include "game.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
static Stage arena() {
    Stage s;
    for (int y = 0; y < MapH; ++y)
        for (int x = 0; x < MapW; ++x)
            s.tiles[y * MapW + x] =
                (x == 0 || y == 0 || x == MapW - 1 || y == MapH - 1) ? Tile::Wall : Tile::Floor;
    return s;
}
static void tick(Game &g, float seconds, Input input = {}) {
    while (seconds > 0 && g.phase == Phase::Playing) {
        float dt = std::min(seconds, .01f);
        g.update(dt, input);
        input.bomb = false;
        seconds -= dt;
    }
}
int main(int argc, char **argv) {
    try {
        if (argc > 1) {
            Stage loaded;
            std::string error;
            check(loadStage(argv[1], loaded, error), "stage file loading");
            check(loaded.enemyCount == 3 && loaded.tiles[11 * MapW + 17] == Tile::Exit,
                  "stage data values");
        }
        Game g(arena());
        tick(g, .1f, {1, 0, false});
        check(g.player.x > 1.5f, "movement");
        tick(g, 1, {-1, 0, false});
        check(g.player.x >= 1.28f, "wall collision");
        g.restart();
        check(g.placeBomb(), "placement");
        check(!g.placeBomb(), "bomb limit");
        tick(g, .4f, {0, 1, false});
        tick(g, .4f, {1, 0, false});
        check(!g.bombs[0].passOwner, "owner leaves bomb");
        check(!g.canStand(1.5f, 1.5f, true), "bomb blocks reentry");
        tick(g, 1.3f);
        check(!g.bombs[0].active && g.fire[MapW + 1] > 0, "timed explosion");
        check(g.phase == Phase::Playing, "escape survives");
        g.restart();
        g.placeBomb();
        tick(g, 2.1f);
        check(g.phase == Phase::Dead, "player death");
        g.restart();
        check(g.phase == Phase::Playing && g.activeBombs() == 0, "restart");
        Stage s = arena();
        s.tiles[MapW + 3] = Tile::Block;
        s.enemyCount = 1;
        s.enemies[0].x = 2.5f;
        s.enemies[0].y = 1.5f;
        GameConfig c;
        c.itemDropRate = 1;
        Game h(s, c);
        h.placeBomb();
        h.player.x = 7.5f;
        h.player.y = 7.5f;
        h.explode(0);
        h.damageAndCollect();
        check(h.stage.tiles[MapW + 3] == Tile::Item, "block drop");
        check(h.fire[MapW + 4] == 0, "block stops blast");
        check(h.aliveEnemies() == 0, "enemy removal");
        for (auto type : {ItemType::Bomb, ItemType::Range, ItemType::Speed}) {
            h.fire = {};
            h.stage.tiles[7 * MapW + 7] = Tile::Item;
            h.stage.items[7 * MapW + 7] = type;
            int b = h.player.maxBombs, r = h.player.range;
            float v = h.player.speed;
            h.damageAndCollect();
            check(h.stage.tiles[7 * MapW + 7] == Tile::Floor, "item collected");
            check(type == ItemType::Bomb    ? h.player.maxBombs == b + 1
                  : type == ItemType::Range ? h.player.range == r + 1
                                            : h.player.speed > v,
                  "item effect");
        }
        Game chain(arena());
        chain.player.maxBombs = 3;
        chain.placeBomb();
        chain.player.x = 3.5f;
        chain.placeBomb();
        chain.player.x = 5.5f;
        chain.placeBomb();
        chain.player.x = 10.5f;
        chain.player.y = 10.5f;
        chain.explode(0);
        check(chain.activeBombs() == 0 && chain.fire[MapW + 7] > 0, "chain explosion");
        Game wall(arena());
        wall.stage.tiles[MapW + 2] = Tile::Wall;
        wall.placeBomb();
        wall.explode(0);
        check(wall.fire[MapW + 2] == 0 && wall.fire[MapW + 3] == 0, "wall stops blast");
        Game win(arena());
        win.stage.tiles[MapW + 1] = Tile::Exit;
        win.damageAndCollect();
        check(win.phase == Phase::Won, "exit victory");
        Stage moving = arena();
        moving.enemyCount = 1;
        moving.enemies[0].x = 8.5f;
        moving.enemies[0].y = 8.5f;
        Game e(moving);
        tick(e, 1);
        check(e.stage.enemies[0].x != 8.5f || e.stage.enemies[0].y != 8.5f, "enemy wandering");
        Stage corridor;
        corridor.tiles.fill(Tile::Wall);
        corridor.tiles[MapW + 1] = Tile::Floor;
        for (int x = 5; x <= 7; ++x)
            corridor.tiles[5 * MapW + x] = Tile::Floor;
        for (int y = 5; y <= 8; ++y)
            corridor.tiles[y * MapW + 7] = Tile::Floor;
        corridor.enemyCount = 1;
        corridor.enemies[0].x = 5.5f;
        corridor.enemies[0].y = 5.5f;
        GameConfig patrolConfig;
        patrolConfig.enemySpeed = 2;
        Game patrol(corridor, patrolConfig);
        tick(patrol, 2);
        check(std::abs(patrol.stage.enemies[0].x - 7.5f) < .02f &&
                  std::abs(patrol.stage.enemies[0].y - 7.5f) < .02f,
              "enemy follows a corner at tile centres");
        tick(patrol, 1.5f);
        check(patrol.stage.enemies[0].y < 7.5f && patrol.stage.enemies[0].x == 7.5f,
              "enemy reverses at dead end");
        patrol.restart();
        tick(patrol, .1f);
        patrol.bombs[0] = {6, 5, 2, patrol.time, 10, true, false};
        tick(patrol, .2f);
        check(patrol.stage.enemies[0].x == 5.5f && patrol.stage.enemies[0].blocked,
              "enemy retreats when a bomb closes its destination");
        patrol.bombs[0].active = false;
        tick(patrol, .25f);
        check(patrol.stage.enemies[0].x > 5.9f, "enemy resumes when passage reopens");
        Game corner(arena());
        corner.stage.tiles[2 * MapW + 2] = Tile::Wall;
        corner.player.x = 1.81f;
        tick(corner, .4f, {0, 1, false});
        check(corner.player.y > 2 && corner.player.x < 1.81f, "player corner alignment correction");
        Game cues(arena());
        cues.update(.016f, {0, 0, true});
        check(cues.events.placed == 1, "placement audio event");
        cues.player.x = 8.5f;
        cues.player.y = 8.5f;
        cues.bombs[0].fuse = .01f;
        cues.update(.016f, {});
        check(cues.events.exploded == 1 && cues.events.placed == 0, "explosion audio event");
        cues.update(.016f, {});
        check(cues.events.exploded == 0, "audio events do not repeat");
        int location = 8 * MapW + 8;
        cues.stage.tiles[location] = Tile::Item;
        cues.stage.items[location] = ItemType::Speed;
        cues.update(.016f, {});
        check(cues.events.collected == 1, "pickup audio event");
        cues.stage.tiles[location] = Tile::Exit;
        cues.update(.016f, {});
        check(cues.events.won, "victory audio event");
        cues.update(.016f, {});
        check(!cues.events.won, "victory cue stops repeating");
        cues.restart();
        cues.fire[MapW + 1] = 1;
        cues.update(.016f, {});
        check(cues.events.died, "death audio event");
        cues.update(.016f, {});
        check(!cues.events.died, "death cue stops repeating");
        std::cout << "PASS: movement, collision, bomb limits/timer, escape, death, restart, "
                     "blocks, enemies, three items, chain, walls, victory, enemy movement, audio "
                     "events\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
