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

// y center of a block in stacking row `row` (0 = on the platform)
inline float rowY(int row, float blockH = BLOCK_UNIT) {
    return PLATFORM_TOP + blockH * 0.5f + row * (blockH + 0.006f);
}

inline vector<LevelDef> makeLevels() {
    const float u = BLOCK_UNIT;
    const Color gold(1.0f, 0.82f, 0.1f);
    vector<LevelDef> levels;

    {   // Level 1: two columns and a lintel beam
        LevelDef l{"FIRST CONTACT", 8, {}};
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
        LevelDef l{"PYRAMID", 8, {}};
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
        LevelDef l{"THE WALL", 14, {}};
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
        LevelDef l{"TWIN TOWERS", 16, {}};
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
        LevelDef l{"TRUSS CASTLE", 20, {}};
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

    return levels;
}
