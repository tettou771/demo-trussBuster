#pragma once

#include <TrussC.h>
#include <vector>

using namespace std;
using namespace tc;

// One block in a level layout. Positions are world space; the tower platform's
// top surface is at y = PLATFORM_TOP.
struct BlockDef {
    Vec3  pos;
    Vec3  size;
    Color color;
    int   points = 100;
    float friction = 0.65f;                  // surface grip (snake segs are slippery)
    bool  wall = false;   // static obstacle: immovable, doesn't count, can't bust
    // kinematic shuttle (moving obstacle; gray like walls, not a target)
    bool     mover = false;
    Vec3     posB;                          // far end of the shuttle run
    float    period = 2.0f;                 // full back-and-forth cycle (s)
    EaseType moveEase = EaseType::Linear;
    bool     piston = false;                // pop-up launcher (pos=retracted, posB=extended, period=bottom dwell)
    Vec3     rotEuler;                      // initial rotation (radians) — ramps etc.
};

// standard look for static obstacles (dark, reads as scenery)
inline Color wallColor() { return Color(0.565f, 0.585f, 0.645f); }

// meshPbr treats baseColor as LINEAR; our palette is authored in sRGB.
// Convert at the material boundary or every color washes out desaturated.
inline Color toLinearColor(const Color& c) {
    return Color(srgbToLinear(c.r), srgbToLinear(c.g), srgbToLinear(c.b), c.a);
}

// global stage-edit flag: while on, physics is paused and blocks must not
// bust themselves (they're being dragged around with the gizmo)
inline bool& stageEditMode() { static bool b = false; return b; }

// Levels with jointed/special structures get built procedurally in GameScene.
enum class Special { None, Boss, WindmillBoss, Bridge, Snake, PinShelf };

struct LevelDef {
    string           name;
    int              shots;
    vector<BlockDef> blocks;
    Special          special = Special::None;
};

constexpr float PLATFORM_TOP = 1.0f;
constexpr float BLOCK_UNIT   = 0.55f;
// platform footprint (must match the StaticProp in GameScene)
constexpr float PLATFORM_HALF_W = 3.2f;
constexpr float PLATFORM_Z_NEAR = -3.9f;
constexpr float PLATFORM_Z_FAR  = -8.1f;

// y center of a block in stacking row `row` (0 = on the platform)
inline float rowY(int row, float blockH = BLOCK_UNIT) {
    return PLATFORM_TOP + blockH * 0.5f + row * (blockH + 0.006f);
}

inline vector<LevelDef> makeLevels() {
    const float u = BLOCK_UNIT;
    const Color gold(1.0f, 0.82f, 0.1f);
    vector<LevelDef> levels;

    {   // Level 1: two columns and a lintel beam
        LevelDef l{"FIRST CONTACT", 6, {}};
        Color mint(0.45f, 0.85f, 0.70f), coral(0.95f, 0.55f, 0.45f);
        for (int col = 0; col < 2; col++) {
            float x = (col == 0) ? -0.62f : 0.62f;
            for (int row = 0; row < 3; row++)
                l.blocks.push_back({Vec3(x, rowY(row), -6.0f), Vec3(u, u, u),
                                    (row % 2 == 0) ? mint : coral, 100});
        }
        float beamH = 0.32f;
        float beamY = PLATFORM_TOP + 3 * (u + 0.006f) + beamH * 0.5f;
        l.blocks.push_back({Vec3(0, beamY, -6.0f), Vec3(1.9f, beamH, 0.5f), gold, 300});
        levels.push_back(l);
    }

    {   // Level 2: pyramid with a gold capstone
        LevelDef l{"PYRAMID", 6, {}};
        Color sand(0.92f, 0.78f, 0.50f), rust(0.85f, 0.50f, 0.30f);
        int widths[] = {4, 3, 2, 1};
        for (int row = 0; row < 4; row++) {
            int w = widths[row];
            for (int i = 0; i < w; i++) {
                float x = (i - (w - 1) * 0.5f) * (u + 0.012f);
                bool top = (row == 3);
                l.blocks.push_back({Vec3(x, rowY(row), -6.0f), Vec3(u, u, u),
                                    top ? Color(1.0f, 0.82f, 0.1f) : (row % 2 ? rust : sand),
                                    top ? 500 : 100});
            }
        }
        levels.push_back(l);
    }

    {   // Level 3: a brick wall
        LevelDef l{"THE WALL", 12, {}};
        Color brickA(0.80f, 0.30f, 0.25f), brickB(0.92f, 0.86f, 0.76f);
        for (int row = 0; row < 4; row++) {
            int  w      = (row % 2 == 0) ? 6 : 5;
            for (int i = 0; i < w; i++) {
                float x = (i - (w - 1) * 0.5f) * (u + 0.012f);
                l.blocks.push_back({Vec3(x, rowY(row), -6.0f), Vec3(u, u, u),
                                    ((row + i) % 2 == 0) ? brickA : brickB, 100});
            }
        }
        levels.push_back(l);
    }

    {   // Level 4: twin towers joined by a beam, gold on the bridge
        LevelDef l{"TWIN TOWERS", 14, {}};
        Color indigo(0.36f, 0.40f, 0.85f), sky(0.55f, 0.78f, 0.95f);
        for (int t = 0; t < 2; t++) {
            float cx = (t == 0) ? -1.7f : 1.7f;
            for (int col = 0; col < 2; col++) {
                float x = cx + ((col == 0) ? -0.29f : 0.29f);
                for (int row = 0; row < 5; row++)
                    l.blocks.push_back({Vec3(x, rowY(row), -6.0f), Vec3(u, u, u),
                                        (row % 2 == 0) ? indigo : sky, 100});
            }
        }
        float beamH = 0.32f;
        float beamY = PLATFORM_TOP + 5 * (u + 0.006f) + beamH * 0.5f;
        l.blocks.push_back({Vec3(0, beamY, -6.0f), Vec3(4.7f, beamH, 0.55f),
                            Color(0.85f, 0.88f, 0.95f), 150});
        l.blocks.push_back({Vec3(0, beamY + beamH * 0.5f + u * 0.5f + 0.006f, -6.0f),
                            Vec3(u, u, u), gold, 500});
        levels.push_back(l);
    }

    {   // Level 5: castle — four corner towers, front wall, center keep + crown
        LevelDef l{"TRUSS CASTLE", 18, {}};
        Color stone(0.62f, 0.62f, 0.68f), slate(0.42f, 0.44f, 0.52f);
        Color royal(0.55f, 0.35f, 0.75f);
        // corner towers (4 high)
        for (float tx : {-2.6f, 2.6f}) {
            for (float tz : {-4.2f, -7.8f}) {
                for (int row = 0; row < 4; row++)
                    l.blocks.push_back({Vec3(tx, rowY(row), tz), Vec3(u, u, u),
                                        (row % 2 == 0) ? stone : slate, 100});
            }
        }
        // front wall (6 wide x 2 high) between the front towers
        for (int row = 0; row < 2; row++) {
            for (int i = 0; i < 6; i++) {
                float x = (i - 2.5f) * (u + 0.012f);
                l.blocks.push_back({Vec3(x, rowY(row), -4.2f), Vec3(u, u, u),
                                    ((row + i) % 2 == 0) ? stone : slate, 100});
            }
        }
        // center keep: 2x2 footprint, 3 high, royal purple
        for (int row = 0; row < 3; row++) {
            for (float kx : {-0.29f, 0.29f}) {
                for (float kz : {-6.2f - 0.29f, -6.2f + 0.29f}) {
                    l.blocks.push_back({Vec3(kx, rowY(row), kz), Vec3(u, u, u),
                                        royal, 150});
                }
            }
        }
        // gold crown on the keep
        l.blocks.push_back({Vec3(0, rowY(3), -6.2f), Vec3(u, u, u), gold, 500});
        levels.push_back(l);
    }

    // ------------------------------------------------------------------
    // Levels 6-10: puzzle stages — destruction ORDER matters.
    // Mechanics verified by MCP playtest (see load_custom_level workflow).
    // ------------------------------------------------------------------

    {   // Level 6 (layout by toru, baked from the live node tree):
        // low wide barriers guard the front edge; corner blues fall to
        // shield-slab caroms, gold pair to a seam shot through the gap
        LevelDef l{"NEEDLE EYE", 6, {}};
        Color shield(0.85f, 0.45f, 0.40f), pairC(0.95f, 0.85f, 0.30f);
        Color single(0.55f, 0.80f, 0.95f);
        l.blocks.push_back({Vec3(-1.0f, 1.55f, -5.0f), Vec3(0.8f, 1.1f, 0.3f), shield, 100});
        l.blocks.push_back({Vec3( 1.0f, 1.55f, -5.0f), Vec3(0.8f, 1.1f, 0.3f), shield, 100});
        l.blocks.push_back({Vec3(-0.30f, 1.25f, -7.3f), Vec3(0.5f, 0.5f, 0.5f), pairC, 200});
        l.blocks.push_back({Vec3( 0.30f, 1.25f, -7.3f), Vec3(0.5f, 0.5f, 0.5f), pairC, 200});
        for (float sgn : {-1.0f, 1.0f}) {
            l.blocks.push_back({Vec3(sgn * 2.2f, 1.4f, -4.1f), Vec3(2.0f, 0.8f, 0.4f),
                                wallColor(), 0, true});
            l.blocks.push_back({Vec3(sgn * 2.3f, 1.2f, -7.55f), Vec3(0.4f, 0.4f, 0.4f),
                                single, 100});
        }
        levels.push_back(l);
    }

    {   // Level 7: two sliding doors (kinematic, linear shuttle) sweep
        // across the lane — time your shots through the moving gap.
        // The nearer door is faster.
        LevelDef l{"FUSUMA", 8, {}};
        Color targetC(0.95f, 0.85f, 0.30f);
        BlockDef doorA;   // front, fast
        doorA.mover = true;
        doorA.pos  = Vec3(-2.6f, 1.75f, -4.3f);
        doorA.posB = Vec3( 2.6f, 1.75f, -4.3f);
        doorA.size = Vec3(1.0f, 1.5f, 0.2f);
        doorA.period = 3.0f;
        l.blocks.push_back(doorA);
        BlockDef doorB;   // behind, slower, starts on the other side
        doorB.mover = true;
        doorB.pos  = Vec3( 2.6f, 1.75f, -5.5f);
        doorB.posB = Vec3(-2.6f, 1.75f, -5.5f);
        doorB.size = Vec3(1.0f, 1.5f, 0.2f);
        doorB.period = 4.0f;
        l.blocks.push_back(doorB);
        // five easy targets in a row behind the doors
        for (float x : {-1.6f, -0.8f, 0.0f, 0.8f, 1.6f})
            l.blocks.push_back({Vec3(x, 1.25f, -7.3f), Vec3(0.5f, 0.5f, 0.5f),
                                targetC, 150});
        levels.push_back(l);
    }

    {   // Level 8: three kinematic decks shuttle above the stage center
        // (Quad ease, gentle enough that riders keep their footing) —
        // bottom is widest+slowest, higher decks are narrower and faster
        LevelDef l{"SKY DECKS", 5, {}};
        Color targetC(0.95f, 0.85f, 0.30f), goldC(1.0f, 0.82f, 0.1f);
        const float deckY[3]   = {1.5f, 3.0f, 4.5f};
        const float amp[3]     = {2.55f, 1.70f, 1.28f};
        const float cycle[3]   = {6.0f, 4.0f, 3.0f};
        const int   pts[3]     = {100, 200, 300};
        for (int i = 0; i < 3; i++) {
            BlockDef deck;
            deck.mover = true;
            float side = (i % 2 == 0) ? -1.0f : 1.0f;
            deck.pos  = Vec3( side * amp[i], deckY[i], -6.0f);
            deck.posB = Vec3(-side * amp[i], deckY[i], -6.0f);
            deck.size = Vec3(1.3f, 0.22f, 1.0f);
            deck.period = cycle[i];
            deck.moveEase = EaseType::Quad;
            l.blocks.push_back(deck);
            // rider on the deck
            l.blocks.push_back({Vec3(side * amp[i], deckY[i] + 0.11f + 0.24f, -6.0f),
                                Vec3(0.45f, 0.45f, 0.45f),
                                i == 2 ? goldC : targetC, pts[i]});
        }
        levels.push_back(l);
    }

    {   // Level 9: skeet range — a stage-wide fence hides three pistons
        // that toss big targets into the air on staggered cycles; shoot them
        // while they fly
        LevelDef l{"MOGURA", 8, {}};
        Color targetC(0.95f, 0.85f, 0.30f), goldC(1.0f, 0.82f, 0.1f);
        // the fence (static): full stage width, hides everything behind it
        l.blocks.push_back({Vec3(0, 1.75f, -5.0f), Vec3(6.4f, 1.5f, 0.2f),
                            wallColor(), 0, true});
        // three pistons + their big riders
        const float px[3]    = {-1.8f, 0.0f, 1.8f};
        const float dwell[3] = {1.6f, 2.3f, 3.0f};   // staggered pop timing
        for (int i = 0; i < 3; i++) {
            BlockDef p;
            p.piston = true;
            p.pos  = Vec3(px[i], 0.4f, -6.2f);       // top flush with the floor
            p.posB = Vec3(px[i], 2.0f, -6.2f);       // top pokes above the fence
            p.size = Vec3(1.7f, 1.2f, 1.7f);   // wide pad: riders re-land on it despite drift
            p.period = dwell[i];
            l.blocks.push_back(p);
            BlockDef r;
            r.pos = Vec3(px[i], 1.0f + 0.33f, -6.2f);
            r.size = Vec3(0.65f, 0.65f, 0.65f);
            r.color = (i == 1) ? goldC : targetC;
            r.points = (i == 1) ? 300 : 150;
            l.blocks.push_back(r);
        }
        // slippery return ramp behind the pads: grazed riders slide back
        // toward the launchers instead of dying unreachable
        BlockDef ramp;
        ramp.wall = true;
        ramp.pos = Vec3(0, 1.40f, -7.78f);
        ramp.size = Vec3(6.4f, 0.12f, 1.6f);
        ramp.rotEuler = Vec3(deg2rad(25.0f), 0, 0);
        ramp.color = wallColor();
        l.blocks.push_back(ramp);
        levels.push_back(l);
    }

    {   // Level 10: ...a boss?! A shabby humanoid robot marches across the
        // stage — kinematic legs (not a target), jointed torso/arms/head.
        // Tear it apart. (Geometry built in GameScene::buildBoss.)
        LevelDef l{"BOSS?!", 6, {}};
        l.special = Special::Boss;
        levels.push_back(l);
    }

    {   // Level 12: the boss carries a motor-driven spinning bar that bats
        // cannonballs away — time your shots through the blade.
        LevelDef l{"WINDMILL", 8, {}};
        l.special = Special::WindmillBoss;
        levels.push_back(l);
    }

    {   // Level 14: a plank bridge hangs between two pylons by block-chain
        // ropes, targets standing on it. Shoot targets directly — or cut a
        // rope and dump everything at once.
        LevelDef l{"ROPEWAY", 4, {}};
        l.special = Special::Bridge;
        levels.push_back(l);
    }

    {   // Level 15: a marching snake of jointed segments. Where you cut
        // matters: sever near the head and the whole body rips free.
        LevelDef l{"SNAKE", 7, {}};
        l.special = Special::Snake;
        levels.push_back(l);
    }

    {   // Level 16: two thin pillars prop a shelf near the edge — no joints,
        // pure stacking. Pick the right pillar and the shelf dumps its gold
        // over the side.
        LevelDef l{"THE SHELF", 3, {}};
        l.special = Special::PinShelf;
        levels.push_back(l);
    }

    return levels;
}
