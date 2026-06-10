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

private:
    Mesh mesh_;
    Material material_;
};
