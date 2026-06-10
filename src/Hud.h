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

    // responsive text factor: 1.0 at >=1100px wide, shrinking to 0.6 on phones
    float uiScale() const { return clamp(getWidth() / 1100.0f, 0.6f, 1.0f); }

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

        // logo with a slow color cycle; never wider than the window
        Color logoCol = Color::fromHSB(fmodf(t * 0.07f, 1.0f), 0.45f, 1.0f);
        float logoScale = std::min(8.0f, (W - 40) / (12 * 8.0f));   // 12 chars
        float H = getHeight();
        centerShadow("TRUSS BUSTER", H * 0.18f, logoScale, logoCol);

        if (fmodf(t, 1.2f) < 0.75f)
            centerShadow(mobile_ ? "TAP TO START" : "PRESS ENTER TO START",
                         H * 0.42f, 3.0f * uiScale(), Color(1.0f, 0.95f, 0.6f));

        center("HI SCORE  " + pad(scene_->getHiScore()), H * 0.51f, 2.0f * uiScale(),
               Color(0.95f, 0.6f, 0.5f));

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
        // responsive: shrink HUD text on narrow (phone) windows...
        float k = uiScale();
        // ...and on REALLY narrow ones, move BLOCKS out of the top-center
        bool narrow = W < 700;
        float ts = 2.0f * k;                 // standard HUD text scale
        float lineH = 13.0f * ts;            // row pitch for stacked lines

        // level (top-left)
        setColor(0.95f, 0.95f, 1.0f);
        drawBitmapString("LEVEL " + to_string(scene_->getLevelNumber()), 24, 22, ts);
        setColor(0.65f, 0.8f, 0.95f);
        drawBitmapString(scene_->getLevelName(), 24, 22 + lineH, ts);

        // score (top-right)
        string sc = "SCORE " + pad(scene_->getScore());
        string hi = "HI    " + pad(scene_->getHiScore());
        setColor(1.0f, 1.0f, 0.95f);
        drawBitmapString(sc, W - textW(sc, ts) - 24, 22, ts);
        setColor(0.8f, 0.65f, 0.6f);
        drawBitmapString(hi, W - textW(hi, ts) - 24, 22 + lineH, ts);

        // shots: cannonball icons, wrapped every 10 (top-left block on mobile,
        // bottom-left on desktop)
        float sx = 24, sy = mobile_ ? 22 + lineH * 2 : H - 52;
        float dotR = 8.0f * k, dotStep = 26.0f * k;
        setColor(0.9f, 0.9f, 0.95f);
        drawBitmapString("SHOTS", sx, sy, ts);
        for (int i = 0; i < scene_->getShots(); i++) {
            setColor(0.85f, 0.88f, 0.95f);
            drawCircle(sx + textW("SHOTS ", ts) + (i % 10) * dotStep,
                       sy + ts * 4.0f + (i / 10) * (dotR * 2.2f), dotR);
        }

        // blocks remaining: top-center normally, stacked under SHOTS when the
        // window is too narrow for three top columns
        string blocks = "BLOCKS " + to_string(scene_->getBlocksLeft()) + "/" +
                        to_string(scene_->getBlocksTotal());
        setColor(0.85f, 0.88f, 0.95f);
        if (narrow) drawBitmapString(blocks, sx, sy + lineH * 1.4f, ts);
        else        center(blocks, 22, ts, Color(0.85f, 0.88f, 0.95f));

        if (scene_->isAutopilot() && fmodf(getElapsedTimef(), 0.8f) < 0.5f)
            center("AUTO", narrow ? 22.0f : 22 + lineH, ts, Color(0.5f, 0.85f, 0.6f));

        // oscillating power gauge while charging — release inside the white
        // MAX zone at the right end for a MAX shot
        Cannon* c = scene_->cannon();
        if (c->isCharging()) {
            float gw = std::min(280.0f, W - 48), gh = 18;
            // mobile: centered above the dpad / fire buttons
            float gx = mobile_ ? (W - gw) * 0.5f : 24;
            float gy = mobile_ ? H - 175 : H - 100;
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

        // (MAX shots are celebrated by sound only — an on-screen banner felt spammy)
    }

    void drawLevelClear() {
        dim(0.45f);
        float t = getElapsedTimef(), k = uiScale(), H = getHeight();
        centerShadow("LEVEL " + to_string(scene_->getLevelNumber()) + " CLEAR!",
                     H * 0.36f, 5.0f * k,
                     Color::fromHSB(fmodf(t * 0.3f, 1.0f), 0.5f, 1.0f));
        center("SHOT BONUS  +" + to_string(scene_->getLastBonus()),
               H * 0.36f + 70 * k, 2.5f * k, Color(1.0f, 0.95f, 0.6f));
        center("SCORE  " + pad(scene_->getScore()),
               H * 0.36f + 110 * k, 2.5f * k, Color(0.95f, 0.95f, 1.0f));
    }

    void drawGameOver() {
        dim(0.55f);
        float k = uiScale(), H = getHeight();
        centerShadow("GAME OVER", H * 0.38f, 6.0f * k, Color(0.95f, 0.35f, 0.3f));
        center("SCORE  " + pad(scene_->getScore()),
               H * 0.38f + 80 * k, 2.5f * k, Color(0.95f, 0.95f, 1.0f));
        if (fmodf(getElapsedTimef(), 1.2f) < 0.75f)
            center(mobile_ ? "TAP TO CONTINUE" : "PRESS ENTER",
                   H * 0.38f + 130 * k, 2.0f * k, Color(0.8f, 0.8f, 0.85f));
    }

    void drawAllClear() {
        dim(0.35f);
        float t = getElapsedTimef(), k = uiScale(), H = getHeight();
        centerShadow("ALL LEVELS CLEAR!", H * 0.28f, 6.0f * k,
                     Color::fromHSB(fmodf(t * 0.5f, 1.0f), 0.7f, 1.0f));
        center("FINAL SCORE", H * 0.28f + 100 * k, 3.0f * k, Color(0.95f, 0.95f, 1.0f));
        centerShadow(pad(scene_->getScore()), H * 0.28f + 150 * k, 5.0f * k,
                     Color(1.0f, 0.85f, 0.3f));
        center("YOU ARE A TRUE TRUSS BUSTER", H * 0.28f + 240 * k, 2.0f * k,
               Color(0.7f, 0.9f, 0.75f));
        if (fmodf(t, 1.2f) < 0.75f)
            center(mobile_ ? "TAP TO CONTINUE" : "PRESS ENTER",
                   H * 0.28f + 290 * k, 2.0f * k, Color(0.8f, 0.8f, 0.85f));
    }
};
