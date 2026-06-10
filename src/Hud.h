#pragma once

#include <TrussC.h>
#include "GameScene.h"

using namespace std;
using namespace tc;

// 2D overlay: title screen, in-game HUD, level clear / game over / all clear.
// Reads game state from the scene; never writes it.
class Hud : public RectNode {
public:
    explicit Hud(GameScene* scene) : scene_(scene) {}

    void setMobile(bool m) { mobile_ = m; }

    void setup() override {
        setName("hud");
        disableEvents();   // never block clicks to the 3D scene / inspector
    }

    void update() override {
        setSize((float)getWindowWidth(), (float)getWindowHeight());
    }

    void draw() override {
        // 2D after 3D: use the alpha-blend pipeline so HUD quads aren't
        // depth-tested against the scene's depth buffer
        setBlendMode(BlendMode::Alpha);
        switch (scene_->getPhase()) {
            case Phase::Title:      drawTitle(); break;
            case Phase::Playing:    drawPlaying(); break;
            case Phase::LevelClear: drawPlaying(); drawLevelClear(); break;
            case Phase::GameOver:   drawPlaying(); drawGameOver(); break;
            case Phase::AllClear:   drawAllClear(); break;
        }
    }

private:
    GameScene* scene_;
    bool       mobile_ = false;

    // bitmap font is 8px monospace at scale 1
    static float textW(const string& s, float scale) { return s.size() * 8.0f * scale; }

    void center(const string& s, float y, float scale, const Color& c) {
        setColor(c);
        drawBitmapString(s, (getWidth() - textW(s, scale)) * 0.5f, y, scale);
    }

    void centerShadow(const string& s, float y, float scale, const Color& c) {
        float x = (getWidth() - textW(s, scale)) * 0.5f;
        setColor(0.0f, 0.0f, 0.0f, 0.55f);
        drawBitmapString(s, x + scale, y + scale, scale);
        setColor(c);
        drawBitmapString(s, x, y, scale);
    }

    static string pad(int v, int digits = 6) {
        string s = to_string(std::max(0, v));
        while ((int)s.size() < digits) s = "0" + s;
        return s;
    }

    void dim(float a) {
        setColor(0.0f, 0.0f, 0.0f, a);
        drawRect(0, 0, getWidth(), getHeight());
    }

    // --- screens ---------------------------------------------------------------
    void drawTitle() {
        float t = getElapsedTimef();
        float W = getWidth();

        // logo with a slow color cycle
        Color logoCol = Color::fromHSB(fmodf(t * 0.07f, 1.0f), 0.45f, 1.0f);
        float logoScale = (W < 760) ? 5.0f : 8.0f;   // fit narrow (phone) windows
        centerShadow("TRUSS BUSTER", 150, logoScale, logoCol);

        if (fmodf(t, 1.2f) < 0.75f)
            centerShadow(mobile_ ? "TAP TO START" : "PRESS ENTER TO START",
                         330, 3, Color(1.0f, 0.95f, 0.6f));

        center("HI SCORE  " + pad(scene_->getHiScore()), 400, 2, Color(0.95f, 0.6f, 0.5f));

        // attract-mode tag
        if (fmodf(t, 0.8f) < 0.5f) {
            setColor(0.5f, 0.85f, 0.6f);
            drawBitmapString("CPU DEMO", W - 110, 20, 1.5f);
        }

        // desktop only: control guide (mobile has on-screen buttons instead)
        if (!mobile_) {
            float y = getHeight() - 92;
            setColor(0.6f, 0.62f, 0.7f);
            drawBitmapString("ARROWS : AIM\nHOLD SPACE : CHARGE & FIRE\nKNOCK EVERY BLOCK OFF THE PLATFORM!", 24, y, 1.5f);
            setColor(0.45f, 0.47f, 0.55f);
            drawBitmapString("F1 : INSPECTOR\nF2 : DEBUG PANEL", getWidth() - 150, y, 1.5f);
        }
    }

    void drawPlaying() {
        float W = getWidth(), H = getHeight();

        // level (top-left)
        setColor(0.95f, 0.95f, 1.0f);
        drawBitmapString("LEVEL " + to_string(scene_->getLevelNumber()), 24, 22, 2.0f);
        setColor(0.65f, 0.8f, 0.95f);
        drawBitmapString(scene_->getLevelName(), 24, 48, 2.0f);

        // score (top-right)
        string sc = "SCORE " + pad(scene_->getScore());
        string hi = "HI    " + pad(scene_->getHiScore());
        setColor(1.0f, 1.0f, 0.95f);
        drawBitmapString(sc, W - textW(sc, 2) - 24, 22, 2.0f);
        setColor(0.8f, 0.65f, 0.6f);
        drawBitmapString(hi, W - textW(hi, 2) - 24, 48, 2.0f);

        // blocks remaining (top-center)
        string blocks = "BLOCKS " + to_string(scene_->getBlocksLeft()) + "/" +
                        to_string(scene_->getBlocksTotal());
        center(blocks, 22, 2, Color(0.85f, 0.88f, 0.95f));

        if (scene_->isAutopilot() && fmodf(getElapsedTimef(), 0.8f) < 0.5f)
            center("AUTO", 50, 2, Color(0.5f, 0.85f, 0.6f));

        // shots: cannonball icons (top-left under the level name on mobile —
        // the bottom-left corner belongs to the dpad there)
        float sx = 24, sy = mobile_ ? 80 : H - 52;
        setColor(0.9f, 0.9f, 0.95f);
        drawBitmapString("SHOTS", sx, sy, 2.0f);
        for (int i = 0; i < scene_->getShots(); i++) {
            setColor(0.85f, 0.88f, 0.95f);
            drawCircle(sx + 111.0f + i * 26.0f, sy + 8.0f, 8.0f);
        }

        // oscillating power gauge while charging — release inside the white
        // MAX zone at the right end for a MAX shot
        Cannon* c = scene_->cannon();
        if (c->isCharging()) {
            float gw = 280, gh = 18;
            // mobile: gauge sits center-bottom, clear of the dpad/fire buttons
            float gx = mobile_ ? (W - gw) * 0.5f : 24;
            float gy = mobile_ ? H - 46 : H - 100;
            float p = c->getPower();
            bool inZone = p >= Cannon::MAX_ZONE;
            setColor(0.85f, 0.85f, 0.9f);
            drawBitmapString("POWER", gx, gy - 22, 1.5f);
            setColor(0.0f, 0.0f, 0.0f, 0.5f);
            drawRect(gx - 4, gy - 4, gw + 8, gh + 8);
            // MAX zone marker
            setColor(1.0f, 1.0f, 1.0f, 0.25f);
            drawRect(gx + gw * Cannon::MAX_ZONE, gy, gw * (1.0f - Cannon::MAX_ZONE), gh);
            if (inZone) setColor(1.0f, 1.0f, 1.0f);   // flash white in the zone
            else        setColor(Color::fromHSB(0.33f * (1.0f - p), 0.85f, 0.95f));
            drawRect(gx, gy, gw * p, gh);
        }

        // MAX shot banner
        float mb = c->getMaxBanner();
        if (mb > 0.0f) {
            float pulse = 3.0f + 0.6f * sinf(getElapsedTimef() * 30.0f);
            center("MAX SHOT!!", H * 0.30f, pulse,
                   Color(1.0f, 0.85f, 0.25f, std::min(1.0f, mb * 2.0f)));
        }
    }

    void drawLevelClear() {
        dim(0.45f);
        float t = getElapsedTimef();
        centerShadow("LEVEL " + to_string(scene_->getLevelNumber()) + " CLEAR!",
                     getHeight() * 0.36f, 5,
                     Color::fromHSB(fmodf(t * 0.3f, 1.0f), 0.5f, 1.0f));
        center("SHOT BONUS  +" + to_string(scene_->getLastBonus()),
               getHeight() * 0.36f + 70, 2.5f, Color(1.0f, 0.95f, 0.6f));
        center("SCORE  " + pad(scene_->getScore()),
               getHeight() * 0.36f + 110, 2.5f, Color(0.95f, 0.95f, 1.0f));
    }

    void drawGameOver() {
        dim(0.55f);
        centerShadow("GAME OVER", getHeight() * 0.38f, 6, Color(0.95f, 0.35f, 0.3f));
        center("SCORE  " + pad(scene_->getScore()),
               getHeight() * 0.38f + 80, 2.5f, Color(0.95f, 0.95f, 1.0f));
        if (fmodf(getElapsedTimef(), 1.2f) < 0.75f)
            center("PRESS ENTER", getHeight() * 0.38f + 130, 2, Color(0.8f, 0.8f, 0.85f));
    }

    void drawAllClear() {
        dim(0.35f);
        float t = getElapsedTimef();
        centerShadow("ALL LEVELS CLEAR!", getHeight() * 0.28f, 6,
                     Color::fromHSB(fmodf(t * 0.5f, 1.0f), 0.7f, 1.0f));
        center("FINAL SCORE", getHeight() * 0.28f + 100, 3, Color(0.95f, 0.95f, 1.0f));
        centerShadow(pad(scene_->getScore()), getHeight() * 0.28f + 150, 5,
                     Color(1.0f, 0.85f, 0.3f));
        center("YOU ARE A TRUE TRUSS BUSTER", getHeight() * 0.28f + 240, 2,
               Color(0.7f, 0.9f, 0.75f));
        if (fmodf(t, 1.2f) < 0.75f)
            center("PRESS ENTER", getHeight() * 0.28f + 290, 2, Color(0.8f, 0.8f, 0.85f));
    }
};
