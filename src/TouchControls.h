#pragma once

#include <TrussC.h>
#include "GameScene.h"

using namespace std;
using namespace tc;

// One semi-transparent on-screen button. Press/release is forwarded to the
// scene as the equivalent key, so touch and keyboard share one code path.
// Works through touch-as-mouse (first finger = mouse) — no touch API needed.
class TouchButton : public RectNode {
public:
    TouchButton(int key, const string& glyph, float glyphScale, GameScene* scene)
        : key_(key), glyph_(glyph), glyphScale_(glyphScale), scene_(scene) {}

    void setup() override {
        setName("btn_" + glyph_);
        enableEvents();
        // a finger sliding off the button still releases it
        releaseL_ = events().mouseReleased.listen([this](MouseEventArgs&) {
            if (pressed_) { pressed_ = false; scene_->handleKey(key_, false); }
        });
    }

    void draw() override {
        setBlendMode(BlendMode::Alpha);
        fill();
        setColor(1.0f, 1.0f, 1.0f, pressed_ ? 0.34f : 0.13f);
        drawRect(0, 0, getWidth(), getHeight());
        setColor(1.0f, 1.0f, 1.0f, pressed_ ? 0.95f : 0.55f);
        float gw = glyph_.size() * 8.0f * glyphScale_;
        drawBitmapString(glyph_, (getWidth() - gw) * 0.5f,
                         (getHeight() - 8.0f * glyphScale_) * 0.5f, glyphScale_);
    }

protected:
    bool onMousePress(const MouseEventArgs&) override {
        pressed_ = true;
        scene_->handleKey(key_, true);
        return true;   // consume: don't orbit the camera under a button
    }

private:
    int           key_;
    string        glyph_;
    float         glyphScale_;
    GameScene*    scene_;
    bool          pressed_ = false;
    EventListener releaseL_;
};

// Mobile control overlay: arrow pad bottom-left, fire button bottom-right.
class TouchControls : public Node {
public:
    explicit TouchControls(GameScene* scene) : scene_(scene) {}

    void setup() override {
        setName("touchControls");
        auto mk = [&](int key, const string& g, float w, float h, float gs) {
            auto b = make_shared<TouchButton>(key, g, gs, scene_);
            b->setSize(w, h);
            addChild(b);
            return b;
        };
        const float s = BTN;
        up_    = mk(KEY_UP,    "^",  s, s, 3.0f);
        down_  = mk(KEY_DOWN,  "v",  s, s, 3.0f);
        left_  = mk(KEY_LEFT,  "<",  s, s, 3.0f);
        right_ = mk(KEY_RIGHT, ">",  s, s, 3.0f);
        fire_  = mk(KEY_SPACE, "FIRE", 150, 120, 3.0f);
    }

    void update() override {
        // re-layout every frame: window size changes on rotation / resize
        float H = (float)getWindowHeight();
        float W = (float)getWindowWidth();
        const float s = BTN, gap = 8;
        float cx = 36 + s * 1.5f + gap;          // dpad center
        float cy = H - 36 - s * 1.5f - gap;
        up_->setPos(cx - s * 0.5f, cy - s * 1.5f - gap);
        down_->setPos(cx - s * 0.5f, cy + s * 0.5f + gap);
        left_->setPos(cx - s * 1.5f - gap, cy - s * 0.5f);
        right_->setPos(cx + s * 0.5f + gap, cy - s * 0.5f);
        fire_->setPos(W - 150 - 30, H - 120 - 40);
    }

private:
    static constexpr float BTN = 76;
    GameScene* scene_;
    shared_ptr<TouchButton> up_, down_, left_, right_, fire_;
};
