#pragma once

#include <TrussC.h>
#include <tcxPhysics.h>
#include "ChipTunes.h"

using namespace std;
using namespace tc;
using namespace tcx;

// A fired cannonball: heavy dynamic sphere, plays impact sounds, expires after
// a few seconds (or when it drops out of the world).
class Cannonball : public Node {
public:
    Cannonball(const Vec3& pos, const Vec3& velocity)
        : startPos_(pos), velocity_(velocity) {}

    void setup() override {
        setName("cannonball");
        setPos(startPos_);
        auto* rb = addMod<RigidBody>(ColliderShape::sphere(0.22f),
                                     BodyType::Dynamic, 9000.0f);
        rb->setFriction(0.5f).setRestitution(0.25f);
        rb->body().setLinearVelocity(velocity_);
        addMod<ColliderRenderer>()->setColor(toLinearColor(Color(0.22f, 0.23f, 0.28f)));
        hitL_ = rb->onCollisionBegan.listen(this, &Cannonball::onHit);
    }

    void update() override {
        age_ += (float)getDeltaTime();
        if (age_ > 5.0f || getGlobalPos().y < -10.0f) destroy();
    }

private:
    void onHit(Collision& c) { jukebox().playImpact(c.speed); }

    Vec3          startPos_, velocity_;
    float         age_ = 0.0f;
    EventListener hitL_;
};
