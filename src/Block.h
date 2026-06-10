#pragma once

#include <TrussC.h>
#include <tcxPhysics.h>
#include "Levels.h"
#include "ChipTunes.h"
#include "FlatRenderer.h"

using namespace std;
using namespace tc;
using namespace tcx;

// A tower block. Self-contained: physics body + renderer + "busted" detection.
// A block counts as busted when it falls off the platform (center drops below
// the platform top); it pops (shrinks) and destroys itself, notifying `busted`
// with its point value.
class Block : public Node {
public:
    explicit Block(const BlockDef& def) : def_(def) {}

    Event<int> busted;   // fired once, with this block's points

    bool isBusted() const { return busted_; }
    int  getPoints() const { return def_.points; }

    using Super = Node;
    TC_REFLECT(Block)
        TC_PROPERTY_RO(points, getPoints)
        TC_PROPERTY_RO(busted, isBusted)
    TC_REFLECT_END

    void setup() override {
        setName(def_.points >= 500 ? "goldBlock" : "block");
        setPos(def_.pos);
        auto* rb = addMod<RigidBody>(ColliderShape::box(def_.size),
                                     BodyType::Dynamic, 800.0f);
        rb->setFriction(0.65f).setRestitution(0.05f);
        renderer_ = addMod<FlatRenderer>();
        renderer_->setColor(def_.color);
    }

    void update() override {
        float dt = (float)getDeltaTime();
        if (!busted_) {
            // fell off the platform -> busted
            if (getGlobalPos().y < PLATFORM_TOP - 0.38f) bust();
        } else {
            // pop: shrink fast, then die
            pop_ -= dt * 3.2f;
            if (pop_ <= 0.05f) { destroy(); return; }
            setScale(pop_, pop_, pop_);
        }
        if (getGlobalPos().y < -12.0f) destroy();   // safety net
    }

private:
    void bust() {
        busted_ = true;
        renderer_->setColor(Color(1.0f, 1.0f, 0.92f));
        if (def_.points >= 500) jukebox().goldBust.play();
        else                    jukebox().bust.play();
        busted.notify(def_.points);
    }

    BlockDef          def_;
    FlatRenderer* renderer_ = nullptr;
    bool              busted_ = false;
    float             pop_ = 1.0f;
};
