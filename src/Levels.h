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
};

struct LevelDef {
    string           name;
    int              shots;
    vector<BlockDef> blocks;
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

    {   // Level 6: thread the gap between the shields, hit pair SEAMS to
        // strike two blocks with one ball
        LevelDef l{"NEEDLE EYE", 4, {}};
        Color shield(0.85f, 0.45f, 0.40f), pairC(0.95f, 0.85f, 0.30f);
        Color single(0.55f, 0.80f, 0.95f);
        l.blocks.push_back({Vec3(-1.0f, 1.55f, -5.0f), Vec3(0.8f, 1.1f, 0.3f), shield, 100});
        l.blocks.push_back({Vec3( 1.0f, 1.55f, -5.0f), Vec3(0.8f, 1.1f, 0.3f), shield, 100});
        // adjacent pair behind the gap: hit the seam, take both
        l.blocks.push_back({Vec3(-0.26f, 1.25f, -7.3f), Vec3(0.5f, 0.5f, 0.5f), pairC, 200});
        l.blocks.push_back({Vec3( 0.26f, 1.25f, -7.3f), Vec3(0.5f, 0.5f, 0.5f), pairC, 200});
        // open-lane singles at the corners (yaw practice)
        // in the crash lane of a knocked shield: carom one slab into one
        l.blocks.push_back({Vec3(-2.3f, 1.2f, -7.5f), Vec3(0.4f, 0.4f, 0.4f), single, 100});
        l.blocks.push_back({Vec3( 2.3f, 1.2f, -7.5f), Vec3(0.4f, 0.4f, 0.4f), single, 100});
        levels.push_back(l);
    }

    {   // Level 7: nine scattered-looking cubes are really THREE depth
        // columns aligned with the cannon (the camera angle hides it) —
        // shoot ALONG a column to tunnel through all three
        LevelDef l{"X-RAY", 5, {}};
        Color amethyst(0.70f, 0.60f, 0.90f);
        const float lanes[3][2] = {{-1.8f, -7.0f}, {0.3f, -6.6f}, {1.6f, -6.9f}};
        for (auto& ln : lanes) {
            float xt = ln[0], zt = ln[1];
            for (int i = -1; i <= 1; i++) {
                float z = zt + i * 0.46f;
                float x = xt * (6.5f - z) / (6.5f - zt);   // on the cannon ray
                l.blocks.push_back({Vec3(x, 1.23f, z), Vec3(0.45f, 0.45f, 0.45f),
                                    amethyst, 100});
            }
        }
        levels.push_back(l);
    }

    {   // Level 8: knock each shield slab so it caroms into the pair behind —
        // the pairs sit straight / right-shifted / left-shifted, so you must
        // hit the right EDGE of each shield
        LevelDef l{"BILLIARDS", 5, {}};
        Color shield(0.85f, 0.45f, 0.40f), pairC(0.95f, 0.85f, 0.30f);
        const float laneX[3] = {-1.9f, 0.0f, 1.9f};
        // pair seams sit on the CANNON RAY through each shield (the cannon is
        // at x=0, so "behind" follows the ray, not straight z), then shifted
        // sideways so each lane needs a different shield-edge hit
        const float pairSeam[3] = {-2.26f, 0.40f, 1.86f};
        for (int i = 0; i < 3; i++) {
            l.blocks.push_back({Vec3(laneX[i], 1.55f, -5.2f), Vec3(0.9f, 1.1f, 0.3f),
                                shield, 100});
            float px = pairSeam[i];
            l.blocks.push_back({Vec3(px - 0.26f, 1.25f, -7.4f), Vec3(0.5f, 0.5f, 0.5f), pairC, 200});
            l.blocks.push_back({Vec3(px + 0.26f, 1.25f, -7.4f), Vec3(0.5f, 0.5f, 0.5f), pairC, 200});
        }
        levels.push_back(l);
    }

    {   // Level 9: seesaw catapults — LOB a ball onto the front end of each
        // beam and the cargo on the rear end launches itself off the platform
        // (hitting the cargo directly only takes the cargo: 3 shots per lane)
        LevelDef l{"CATAPULT", 6, {}};
        Color fulcrumC(0.55f, 0.55f, 0.65f), beamC(0.90f, 0.80f, 0.40f);
        Color cargoC(0.95f, 0.85f, 0.30f);
        for (float cx : {-1.5f, 1.5f}) {
            l.blocks.push_back({Vec3(cx, 1.25f, -6.4f), Vec3(0.5f, 0.5f, 0.6f), fulcrumC, 100});
            l.blocks.push_back({Vec3(cx, 1.63f, -6.4f), Vec3(0.7f, 0.22f, 2.6f), beamC, 150});
            l.blocks.push_back({Vec3(cx, 1.95f, -7.3f), Vec3(0.45f, 0.45f, 0.45f), cargoC, 200});
            l.blocks.push_back({Vec3(cx, 1.95f, -7.0f), Vec3(0.45f, 0.45f, 0.45f), cargoC, 200});
        }
        // center: plain seam pair as a breather
        l.blocks.push_back({Vec3(-0.26f, 1.25f, -7.4f), Vec3(0.5f, 0.5f, 0.5f), cargoC, 200});
        l.blocks.push_back({Vec3( 0.26f, 1.25f, -7.4f), Vec3(0.5f, 0.5f, 0.5f), cargoC, 200});
        levels.push_back(l);
    }

    {   // Level 10: the final exam — dominoes into the gold, shielded pair,
        // and a plow lane, all on one platform
        LevelDef l{"MASTERPIECE", 7, {}};
        Color domC(0.60f, 0.85f, 0.50f);
        Color shield(0.85f, 0.45f, 0.40f), pairC(0.95f, 0.85f, 0.30f);
        Color wallC(0.90f, 0.80f, 0.40f), rowC(0.50f, 0.75f, 0.95f);
        // center: domino run ending at the gold block
        for (float z : {-4.8f, -5.9f, -7.0f})
            l.blocks.push_back({Vec3(0, 1.7f, z), Vec3(1.0f, 1.4f, 0.25f), domC, 100});
        l.blocks.push_back({Vec3(0, 1.25f, -7.9f), Vec3(0.5f, 0.5f, 0.5f), gold, 500});
        // left: shield + seam pair
        l.blocks.push_back({Vec3(-1.9f, 1.55f, -5.2f), Vec3(1.0f, 1.1f, 0.3f), shield, 100});
        l.blocks.push_back({Vec3(-2.16f, 1.25f, -7.5f), Vec3(0.5f, 0.5f, 0.5f), pairC, 200});
        l.blocks.push_back({Vec3(-1.64f, 1.25f, -7.5f), Vec3(0.5f, 0.5f, 0.5f), pairC, 200});
        // right: mini plow
        l.blocks.push_back({Vec3(1.9f, 1.65f, -4.7f), Vec3(1.6f, 1.3f, 0.3f), wallC, 150});
        for (float x : {1.4f, 1.9f, 2.4f})
            l.blocks.push_back({Vec3(x, 1.2f, -7.7f), Vec3(0.4f, 0.4f, 0.4f), rowC, 100});
        levels.push_back(l);
    }

    return levels;
}
