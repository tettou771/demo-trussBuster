#pragma once

#include <TrussC.h>
#include <tcxPhysics.h>
#include "Levels.h"
#include "ChipTunes.h"

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
    void setPoints(int p) { def_.points = p; }
    Vec3 getSize() const { return def_.size; }
    void setSize(const Vec3& s) {
        def_.size = s;
        if (!renderer_) return;   // before setup: just store
        // re-create the body with the new collider (addMod replaces the old
        // mod; its onDestroy removes the old body from the world)
        auto* rb = addMod<RigidBody>(ColliderShape::box(s), BodyType::Dynamic, 800.0f);
        rb->setFriction(0.65f).setRestitution(0.05f);
        renderer_ = addMod<ColliderRenderer>();   // recreate: drops the cached mesh
        renderer_->setColor(toLinearColor(def_.color));
    }
    bool getFalse() const { return false; }
    void doDelete(bool v) { if (v) destroy(); }   // inspector checkbox = button

    using Super = Node;
    TC_REFLECT(Block, Node) {
        TC_VALUE(points, getPoints, setPoints)
        TC_VALUE(size, getSize, setSize)
        TC_VALUE(busted, isBusted)
        TC_VALUE(deleteBlock, getFalse, doDelete)
    }

    void setup() override {
        setName(def_.points >= 500 ? "goldBlock" : "block");
        setPos(def_.pos);
        auto* rb = addMod<RigidBody>(ColliderShape::box(def_.size),
                                     BodyType::Dynamic, 800.0f);
        rb->setFriction(0.65f).setRestitution(0.05f);
        renderer_ = addMod<ColliderRenderer>();
        renderer_->setColor(toLinearColor(def_.color));

        // Whenever the NODE moves, push the transform into the BODY — but
        // only when the change came from OUTSIDE (gizmo/inspector/setPos).
        // RigidBody::isSyncing() identifies its own body->node sync writes,
        // which must NOT bounce back (that froze all rotation once).
        moveL_ = localMatrixChanged.listen([this]() { pushToBody(); });
    }

    void pushToBody() {
        auto* rb = getMod<RigidBody>();
        if (!rb || !rb->body().isValid() || rb->isSyncing()) return;
        // external edit: teleport the body (velocity preserved)
        rb->body().setPosition(getGlobalPos());
        rb->body().setRotation(getQuaternion());
    }

    void update() override {
        if (stageEditMode()) return;   // being edited: never self-bust
        float dt = (float)getDeltaTime();
        if (!busted_) {
            // busted when it leaves the platform: below its top, OR outside
            // its footprint near ground level (tall blocks lying on the
            // ground can keep their center above the simple y threshold)
            Vec3 gp = getGlobalPos();
            bool below = gp.y < PLATFORM_TOP - 0.38f;
            bool outside = (fabsf(gp.x) > PLATFORM_HALF_W + 0.15f ||
                            gp.z > PLATFORM_Z_NEAR + 0.15f ||
                            gp.z < PLATFORM_Z_FAR - 0.15f) && gp.y < 0.95f;
            if (below || outside) bust();
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
        renderer_->setColor(toLinearColor(Color(1.0f, 1.0f, 0.92f)));
        if (def_.points >= 500) jukebox().goldBust.play();
        else                    jukebox().bust.play();
        busted.notify(def_.points);
    }

    BlockDef          def_;
    ColliderRenderer* renderer_ = nullptr;
    bool              busted_ = false;
    float             pop_ = 1.0f;
    EventListener     moveL_;
};
