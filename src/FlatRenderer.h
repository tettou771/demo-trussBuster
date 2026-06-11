#pragma once

#include <TrussC.h>
#include <tcxPhysics.h>

using namespace tc;
using namespace tcx;

// Renders the sibling RigidBody's collider mesh with CPU (per-vertex)
// lighting through sokol_gl, instead of ColliderRenderer's GPU PBR path.
//
// Why not ColliderRenderer: the meshPbr shader currently kills the GPU
// context on iOS Safari (wasm target of this game), and deferred PBR draws
// can overdraw HUD text. Per-vertex lighting is visually equivalent for
// flat-shaded boxes, and the whole scene is a few hundred vertices.
class FlatRenderer : public Mod {
    friend class trussc::Node;

public:
    // baseColor + a matching emissive floor (fake ambient: CPU lighting has
    // no ambient term, so faces away from every light would go pitch black)
    FlatRenderer& setColor(const Color& c) {
        material_.setBaseColor(c);
        material_.setEmissive(c);
        material_.setEmissiveStrength(0.12f);
        return *this;
    }
    Color getColor() const { return material_.getBaseColor(); }

    // drop the cached mesh; it rebuilds from the (possibly new) collider
    // shape on the next draw
    void invalidateMesh() { mesh_ = Mesh(); }

    using Super = Mod;
    TC_REFLECT(FlatRenderer)
        TC_PROPERTY(color, getColor, setColor)
    TC_REFLECT_END

protected:
    void draw() override {
        if (mesh_.getNumVertices() == 0) {
            Node* n = getOwner();
            if (!n->hasMod<RigidBody>()) return;
            mesh_ = buildShapeMesh(n->getMod<RigidBody>()->shape());
            if (mesh_.getNumVertices() == 0) return;
        }
        setMaterial(material_);
        mesh_.drawWithLighting();
        clearMaterial();
    }

    bool isExclusive() const override { return true; }

    // Mouse picking: ray vs the collider's bounding box in local space, so
    // blocks/walls/balls are clickable in the inspector (plain Nodes have no
    // hit geometry of their own).
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

private:
    Mesh mesh_;
    Material material_;
};
