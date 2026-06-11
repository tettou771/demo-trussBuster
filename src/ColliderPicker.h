#pragma once

#include <TrussC.h>
#include <tcxPhysics.h>

using namespace tc;
using namespace tcx;

// Makes a physics node mouse-pickable: Mod::hitTest against the sibling
// RigidBody's collider bounding box (plain Nodes have no hit geometry, so
// without this, clicks pass straight through blocks in the inspector).
//
// This is the Mod-side of the hit-test hook: returning false = pass-through,
// returning true (+distance) = participate. Today core ORs all mod results
// with the node's own hitTest; there is no veto/AND combinator yet.
class ColliderPicker : public Mod {
    friend class trussc::Node;

public:
    bool hitTest(const Ray& localRay, float& outDistance) override {
        Node* n = getOwner();
        if (!n->hasMod<RigidBody>()) return false;
        const ColliderShape& s = n->getMod<RigidBody>()->shape();
        Vec3 half;
        switch (s.kind) {
            case ColliderShape::Box:      half = s.size * 0.5f; break;
            case ColliderShape::Sphere:   half = Vec3(s.radius, s.radius, s.radius); break;
            case ColliderShape::Capsule:
            case ColliderShape::Cylinder: half = Vec3(s.radius, s.height * 0.5f + s.radius, s.radius); break;
            default: return false;
        }
        // slab test against the AABB [-half, +half]
        float tmin = -1e30f, tmax = 1e30f;
        const float o[3] = {localRay.origin.x, localRay.origin.y, localRay.origin.z};
        const float d[3] = {localRay.direction.x, localRay.direction.y, localRay.direction.z};
        const float h[3] = {half.x, half.y, half.z};
        for (int i = 0; i < 3; i++) {
            if (fabsf(d[i]) < 1e-8f) {
                if (o[i] < -h[i] || o[i] > h[i]) return false;
                continue;
            }
            float t1 = (-h[i] - o[i]) / d[i];
            float t2 = ( h[i] - o[i]) / d[i];
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return false;
        }
        if (tmax < 0) return false;          // box behind the ray
        outDistance = tmin >= 0 ? tmin : tmax;
        return true;
    }

    bool isExclusive() const override { return true; }
};
