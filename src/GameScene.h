#pragma once

#include <TrussC.h>
#include <tcxPhysics.h>
#include "ChipTunes.h"
#include "Levels.h"
#include "Block.h"
#include "Cannonball.h"
#include "Cannon.h"

using namespace std;
using namespace tc;
using namespace tcx;

enum class Phase { Title, Playing, LevelClear, GameOver, AllClear };

inline const char* phaseName(Phase p) {
    switch (p) {
        case Phase::Title:      return "Title";
        case Phase::Playing:    return "Playing";
        case Phase::LevelClear: return "LevelClear";
        case Phase::GameOver:   return "GameOver";
        case Phase::AllClear:   return "AllClear";
    }
    return "?";
}

// A static physics box with a renderer (platform, pedestal, level walls).
class StaticProp : public Node {
public:
    StaticProp(const Vec3& pos, const Vec3& size, const Color& color)
        : pos_(pos), size_(size), color_(color) {}

    Vec3 getSize() const { return size_; }
    void setSize(const Vec3& s) {
        size_ = s;
        if (!renderer_) return;   // before setup: just store
        addMod<RigidBody>(ColliderShape::box(s), BodyType::Kinematic);
        renderer_ = addMod<ColliderRenderer>();   // recreate: drops the cached mesh
        renderer_->setColor(toLinearColor(color_));
    }
    bool getFalse() const { return false; }
    void doDelete(bool v) { if (v) destroy(); }   // inspector checkbox = button

    using Super = Node;
    TC_REFLECT(StaticProp)
        TC_PROPERTY(size, getSize, setSize)
        TC_PROPERTY(deleteBlock, getFalse, doDelete)
    TC_REFLECT_END

    void setup() override {
        setPos(pos_);
        // Kinematic, not Static: RigidBody's kinematic mode syncs node->body
        // every frame, so gizmo/inspector edits move the COLLIDER too (a
        // static body would leave its collision behind)
        addMod<RigidBody>(ColliderShape::box(size_), BodyType::Kinematic);
        renderer_ = addMod<ColliderRenderer>();
        renderer_->setColor(toLinearColor(color_));
    }

private:
    Vec3 pos_, size_;
    Color color_;
    ColliderRenderer* renderer_ = nullptr;
};

// Parent node for level blocks. Reflected spawn buttons (checkbox = button):
// set spawnSize/spawnPoints, then tick spawnBlock / spawnWall in the
// inspector — a fresh object appears at the stage center, ready to be moved
// with the gizmo (turn editMode on first so physics doesn't fight you).
class TowerRoot : public Node {
public:
    Event<BlockDef> spawnReq;   // GameScene listens and does the spawning

    Vec3 spawnSize  = Vec3(0.5f, 0.5f, 0.5f);
    int  spawnPoints = 100;

    bool getFalse() const { return false; }
    void doSpawnBlock(bool v) { if (v) request(false); }
    void doSpawnWall(bool v)  { if (v) request(true); }

    using Super = Node;
    TC_REFLECT(TowerRoot)
        TC_FIELD(spawnSize)
        TC_FIELD(spawnPoints)
        TC_PROPERTY(spawnBlock, getFalse, doSpawnBlock)
        TC_PROPERTY(spawnWall, getFalse, doSpawnWall)
    TC_REFLECT_END

private:
    void request(bool wall) {
        BlockDef d;
        d.size = spawnSize;
        d.pos = Vec3(0, PLATFORM_TOP + spawnSize.y * 0.5f + 0.01f, -6.0f);
        d.points = spawnPoints;
        d.wall = wall;
        d.color = wall ? wallColor() : Color(0.95f, 0.85f, 0.30f);
        spawnReq.notify(d);
    }
};

// The 3D world: camera scope, physics, cannon, towers, game state machine.
class GameScene : public Node {
public:
    // --- state accessors (HUD / MCP) ----------------------------------------
    Phase  getPhase() const      { return phase_; }
    int    getScore() const      { return score_; }
    int    getHiScore() const    { return hiScore_; }
    int    getShots() const      { return shots_; }
    int    getBlocksTotal() const { return blocksTotal_; }
    int    getBlocksLeft() const { return blocksLeft_; }
    int    getLevelNumber() const { return (int)levelIdx_ + 1; }
    int    getLastBonus() const  { return lastBonus_; }
    bool   isAutopilot() const   { return autopilot_; }
    const string& getLevelName() const { return levels_[levelIdx_].name; }
    Cannon* cannon()             { return cannon_.get(); }

    // --- stage-editing debug API (inspector / MCP) ----------------------------
    bool isEditMode() const { return editMode_; }
    void setEditMode(bool on) {
        editMode_ = on;
        stageEditMode() = on;
        if (!on) {
            // freeze all bodies so the resumed simulation starts calm
            for (auto& c : towerRoot_->getChildren()) {
                if (c->isDead()) continue;
                if (auto* rb = c->getMod<RigidBody>()) {
                    rb->body().setLinearVelocity(Vec3(0, 0, 0));
                    rb->body().setAngularVelocity(Vec3(0, 0, 0));
                }
            }
        }
    }

    bool getFalse() const { return false; }
    void doReload(bool v) { if (v) loadLevel(levelIdx_); }   // reset shots+blocks

    using Super = Node;
    TC_REFLECT(GameScene)
        TC_PROPERTY_RO(score, getScore)
        TC_PROPERTY_RO(shots, getShots)
        TC_PROPERTY_RO(blocksLeft, getBlocksLeft)
        TC_PROPERTY(level, getLevelNumber, gotoLevel)
        TC_PROPERTY(editMode, isEditMode, setEditMode)
        TC_PROPERTY(reloadLevel, getFalse, doReload)
    TC_REFLECT_END

    int aliveBalls() const {
        int n = 0;
        for (auto& c : ballsRoot_->getChildren())
            if (!c->isDead()) n++;
        return n;
    }

    json stateJson() {
        return json{
            {"phase", phaseName(phase_)},
            {"level", getLevelNumber()},
            {"levelName", getLevelName()},
            {"score", score_},
            {"hiScore", hiScore_},
            {"shots", shots_},
            {"blocksTotal", blocksTotal_},
            {"blocksLeft", blocksLeft_},
            {"ballsInFlight", aliveBalls()},
            {"autopilot", autopilot_},
            {"cannon", {
                {"yawDeg", cannon_->getYawDeg()},
                {"pitchDeg", cannon_->getPitchDeg()},
                {"power", cannon_->getPower()},
                {"charging", cannon_->isCharging()},
            }},
        };
    }

    // --- commands (keys / ImGui / MCP) ---------------------------------------
    void startGame() {
        score_ = 0;
        levelIdx_ = 0;
        loadLevel(0);
        phase_ = Phase::Playing;
        jukebox().startJingle.play();
        if (!jukebox().bgm.isPlaying()) jukebox().bgm.play();
    }

    void toTitle() {
        phase_ = Phase::Title;
        levelIdx_ = 2;          // attract demo plays on THE WALL
        loadLevel(levelIdx_);
        aiTimer_ = 1.5f;        // demo starts shooting soon
        if (!jukebox().bgm.isPlaying()) jukebox().bgm.play();
    }

    void requestFire(float power) {
        if (phase_ == Phase::Playing) {
            if (shots_ <= 0) { jukebox().dryFire.play(); return; }
            cannon_->fire(power);
        } else if (phase_ == Phase::Title) {
            cannon_->fire(power);
        }
    }

    void setAim(float yawDeg, float pitchDeg) {
        cannon_->setYawDeg(yawDeg);
        cannon_->setPitchDeg(pitchDeg);
    }

    void setAutopilot(bool on) { autopilot_ = on; aiTimer_ = 0; aiAiming_ = false; }

    // replace a level definition at runtime (MCP stage-design workflow:
    // prototype layouts as level 1 without rebuilding, then bake into Levels.h)
    void setLevelDef(int oneBased, LevelDef def) {
        size_t i = (size_t)clamp(oneBased - 1, 0, (int)levels_.size() - 1);
        levels_[i] = std::move(def);
    }

    // jump straight into a level (debug / MCP)
    void gotoLevel(int oneBased) {
        levelIdx_ = (size_t)clamp(oneBased - 1, 0, (int)levels_.size() - 1);
        loadLevel(levelIdx_);
        phase_ = Phase::Playing;
        if (!jukebox().bgm.isPlaying()) jukebox().bgm.play();
    }

    void handleKey(int key, bool down) {
        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN) {
            held_[key] = down;
            return;
        }
        if (down && key == KEY_ENTER) {
            if (phase_ == Phase::Title)         startGame();
            else if (phase_ == Phase::GameOver) toTitle();
            else if (phase_ == Phase::AllClear) toTitle();
            return;
        }
        if (key == KEY_SPACE && phase_ == Phase::Playing && !autopilot_) {
            if (down && !cannon_->isCharging()) {
                if (shots_ > 0) cannon_->beginCharge();
                else            jukebox().dryFire.play();
            } else if (!down && cannon_->isCharging()) {
                cannon_->releaseCharge();
            }
        }
    }

    // --- lifecycle ------------------------------------------------------------
    void setup() override {
        setName("scene");

        defaultWorld().setup();
        defaultWorld().setGravity(Vec3(0, -12.0f, 0));
        defaultWorld().addGroundPlane(0.0f);

        // fixed camera for play; SHIFT+drag orbits (debug). The modifier keeps
        // plain clicks unconsumed so touch buttons / node picking still work.
        cam_.setTarget(0.0f, 1.4f, -1.5f);
        cam_.setDistance(16.2f);   // pulled back ~20%: whole stage + barrel visible
        cam_.setAzimuth(0.22f);
        cam_.setElevation(0.30f);
        cam_.enableMouseInput();
        cam_.setDragModifier(EasyCam::Modifier::Shift);

        keyLight_.setDirectional(Vec3(-0.4f, -1.0f, -0.55f));
        keyLight_.setDiffuse(1.0f, 0.96f, 0.88f);
        keyLight_.setIntensity(2.4f);
        addLight(keyLight_);

        fillLight_.setDirectional(Vec3(0.6f, -0.25f, 0.5f));
        fillLight_.setDiffuse(0.55f, 0.58f, 0.70f);
        fillLight_.setIntensity(1.1f);
        addLight(fillLight_);

        // narrow depth: hit blocks should fall off the back edge easily
        // common static-prop color (matches level walls — "fixed scenery" look)
        auto platform = make_shared<StaticProp>(Vec3(0, 0.5f, -6.0f), Vec3(6.4f, 1, 4.2f),
                                                wallColor());
        platform->setName("platform");
        addChild(platform);

        auto pedestal = make_shared<StaticProp>(Vec3(0, 0.25f, 6.5f), Vec3(1.5f, 0.5f, 1.5f),
                                                Color(0.42f, 0.44f, 0.50f));
        pedestal->setName("pedestal");
        addChild(pedestal);

        cannon_ = make_shared<Cannon>();
        cannon_->setPos(0, 0.5f, 6.5f);
        addChild(cannon_);

        towerRoot_ = make_shared<TowerRoot>();
        towerRoot_->setName("tower");
        addChild(towerRoot_);
        spawnL_ = towerRoot_->spawnReq.listen(this, &GameScene::onSpawnReq);

        ballsRoot_ = make_shared<Node>();
        ballsRoot_->setName("balls");
        addChild(ballsRoot_);

        firedL_ = cannon_->fired.listen(this, &GameScene::onFired);

        levels_ = makeLevels();
        toTitle();
    }

    void update() override {
        float dt = std::min(0.05f, std::max(0.0f, (float)getDeltaTime()));

        if (editMode_) {
            // stage editing: physics paused; push gizmo/inspector edits of the
            // node transforms INTO the bodies (normally RigidBody syncs the
            // other way and would overwrite them)
            for (auto& c : towerRoot_->getChildren()) {
                if (c->isDead()) continue;
                if (auto* rb = c->getMod<RigidBody>()) {
                    rb->body().setPosition(c->getPos());
                    rb->body().setRotation(c->getQuaternion());
                    rb->body().setLinearVelocity(Vec3(0, 0, 0));
                    rb->body().setAngularVelocity(Vec3(0, 0, 0));
                }
            }
            return;   // no sim, no autopilot, no game-over logic
        }

        defaultWorld().update(dt);

        // manual aiming with held arrow keys
        if (phase_ == Phase::Playing && !autopilot_) {
            float aimRate = 0.2f;   // rad/s — slow enough for fine aiming
            if (held_[KEY_LEFT])  cannon_->setYaw(cannon_->getYaw() + aimRate * dt);
            if (held_[KEY_RIGHT]) cannon_->setYaw(cannon_->getYaw() - aimRate * dt);
            if (held_[KEY_UP])    cannon_->setPitch(cannon_->getPitch() + aimRate * dt);
            if (held_[KEY_DOWN])  cannon_->setPitch(cannon_->getPitch() - aimRate * dt);
        }

        // autopilot: attract demo on the title, optional CPU player in game
        if (phase_ == Phase::Title || (phase_ == Phase::Playing && autopilot_)) {
            updateAutopilot(dt);
        }

        // out of shots, nothing in flight -> wait for the dust to settle
        if (phase_ == Phase::Playing && shots_ <= 0 && aliveBalls() == 0 &&
            !cannon_->isCharging() && blocksLeft_ > 0) {
            settle_ += dt;
            if (settle_ > 2.8f) gameOver();
        } else {
            settle_ = 0.0f;
        }

        // attract demo: when the demo tower is cleared, rebuild it
        if (phase_ == Phase::Title && blocksLeft_ <= 0) {
            demoReload_ += dt;
            if (demoReload_ > 2.0f) {
                demoReload_ = 0;
                loadLevel(levelIdx_);
            }
        }
    }

    void draw() override {
        // ground grid
        setColor(0.22f, 0.23f, 0.30f);
        const float ext = 14.0f, stp = 1.0f;
        for (float a = -ext; a <= ext + 0.001f; a += stp) {
            drawLine(Vec3(a, 0.005f, -ext), Vec3(a, 0.005f, ext));
            drawLine(Vec3(-ext, 0.005f, a), Vec3(ext, 0.005f, a));
        }

    }

protected:
    void beginDraw() override {
        cam_.begin();
        setCameraPosition(cam_.getPosition());
    }

    void endDraw() override {
        // aim guide: drawn AFTER all children so the dots overlay the blocks
        // (they don't depth-test, so drawing first would let blocks paint over
        // them), but still inside the camera scope
        if (phase_ == Phase::Playing && !autopilot_) drawTrajectory();
        cam_.end();
    }

private:
    // --- internals ------------------------------------------------------------
    // Sample the MAX-power ballistic arc and place small spheres at equal
    // arc-length intervals (a dotted line in 3D), stopping at the first
    // physics hit (raycast per segment) with an impact marker.
    void drawTrajectory() {
        if (dotMesh_.getNumVertices() == 0) dotMesh_ = createSphere(0.06f, 10);
        Vec3  p = cannon_->muzzlePos();
        Vec3  v = cannon_->aimDir() * Cannon::MAX_SPEED;
        float g = defaultWorld().getGravity().y;   // negative
        // default (depth-tested) pipeline: dots passing BEHIND blocks are
        // correctly hidden — drawing them additive/depth-less made the guide
        // float confusingly over everything
        setColor(1.0f, 0.9f, 0.45f, 0.6f);
        const float step = 0.02f;     // integration step (s)
        const float gap  = 0.55f;     // distance between dots (m)
        float acc = gap;              // place the first dot immediately
        Vec3  prev = p;
        int   dots = 0;
        for (float t = step; t < 3.0f && dots < 48; t += step) {
            Vec3 cur = p + v * t + Vec3(0, 0.5f * g * t * t, 0);
            Vec3 seg = cur - prev;
            float len = seg.length();
            if (len > 1e-6f) {
                if (auto h = defaultWorld().raycast(prev, seg / len, len)) {
                    // impact marker: one brighter, bigger dot, then stop
                    setColor(1.0f, 0.85f, 0.45f, 0.9f);
                    pushMatrix();
                    translate(h.point);
                    scale(1.8f, 1.8f, 1.8f);
                    dotMesh_.draw();
                    popMatrix();
                    break;
                }
            }
            acc += len;
            prev = cur;
            if (acc >= gap) {
                acc = 0;
                dots++;
                pushMatrix();
                translate(cur);
                dotMesh_.draw();
                popMatrix();
            }
            if (cur.y < 0.05f) break;
        }
        setColor(1.0f);
    }

    void loadLevel(size_t idx) {
        for (auto& c : towerRoot_->getChildren()) c->destroy();
        for (auto& c : ballsRoot_->getChildren()) c->destroy();
        blockL_.clear();

        const LevelDef& def = levels_[idx];
        int targets = 0;
        for (const auto& bd : def.blocks) {
            if (bd.wall) {
                // static obstacle: immovable scenery, not a target
                auto wall = make_shared<StaticProp>(bd.pos, bd.size, bd.color);
                wall->setName("wall");
                towerRoot_->addChild(wall);
                continue;
            }
            auto block = make_shared<Block>(bd);
            blockL_.push_back(block->busted.listen(this, &GameScene::onBlockBusted));
            towerRoot_->addChild(block);
            targets++;
        }
        shots_ = def.shots;
        blocksTotal_ = targets;
        blocksLeft_ = blocksTotal_;
        lastBonus_ = 0;
        settle_ = 0;
        aiAiming_ = false;
        aiTimer_ = 0;
        cannon_->cancelCharge();
        cannon_->setYawDeg(0);
        cannon_->setPitchDeg(20);
    }

    // spawn-button handler (stage editing): drop a fresh block/wall mid-stage
    void onSpawnReq(BlockDef& bd) {
        if (bd.wall) {
            auto wall = make_shared<StaticProp>(bd.pos, bd.size, bd.color);
            wall->setName("wall");
            towerRoot_->addChild(wall);
        } else {
            auto block = make_shared<Block>(bd);
            blockL_.push_back(block->busted.listen(this, &GameScene::onBlockBusted));
            towerRoot_->addChild(block);
            blocksTotal_++;
            blocksLeft_++;
        }
    }

    void onFired(Cannon::FireArgs& args) {
        ballsRoot_->addChild(make_shared<Cannonball>(args.pos, args.velocity));
        if (phase_ == Phase::Playing) {
            shots_--;
            settle_ = 0;
        }
    }

    void onBlockBusted(int& points) {
        blocksLeft_--;
        if (phase_ == Phase::Playing) {
            score_ += points;
            hiScore_ = std::max(hiScore_, score_);
            if (blocksLeft_ <= 0) levelClear();
        }
    }

    void levelClear() {
        phase_ = Phase::LevelClear;
        lastBonus_ = shots_ * 200;
        score_ += lastBonus_;
        hiScore_ = std::max(hiScore_, score_);
        jukebox().clearJingle.play();
        callAfter(3.0, [this]() {
            if (phase_ != Phase::LevelClear) return;
            if (levelIdx_ + 1 < levels_.size()) {
                levelIdx_++;
                loadLevel(levelIdx_);
                phase_ = Phase::Playing;
                jukebox().startJingle.play();
            } else {
                allClear();
            }
        });
    }

    void gameOver() {
        phase_ = Phase::GameOver;
        jukebox().bgm.stop();
        jukebox().overJingle.play();
        callAfter(4.5, [this]() {
            if (phase_ == Phase::GameOver) toTitle();
        });
    }

    void allClear() {
        phase_ = Phase::AllClear;
        jukebox().bgm.stop();
        jukebox().fanfare.play();
    }

    // --- autopilot -------------------------------------------------------------
    void updateAutopilot(float dt) {
        if (phase_ == Phase::Playing && shots_ <= 0) return;

        if (!aiAiming_) {
            aiTimer_ += dt;
            if (aiTimer_ < aiInterval_) return;
            if (pickTargetAndSolve()) {
                aiAiming_ = true;
            } else {
                aiTimer_ = 0;   // nothing to shoot at yet
            }
            return;
        }

        // ease the barrel toward the solution, then fire
        float rate = 2.2f * dt;
        float y = cannon_->getYaw(), p = cannon_->getPitch();
        y += clamp(aiYaw_ - y, -rate, rate);
        p += clamp(aiPitch_ - p, -rate, rate);
        cannon_->setYaw(y);
        cannon_->setPitch(p);
        if (fabsf(aiYaw_ - y) < 0.008f && fabsf(aiPitch_ - p) < 0.008f) {
            requestFire(aiPower_);
            aiAiming_ = false;
            aiTimer_ = 0;
            aiInterval_ = random(1.8f, 2.6f);
        }
    }

    bool pickTargetAndSolve() {
        // collect alive, un-busted blocks
        vector<Block*> alive;
        for (auto& c : towerRoot_->getChildren()) {
            if (c->isDead()) continue;
            auto* b = dynamic_cast<Block*>(c.get());
            if (b && !b->isBusted()) alive.push_back(b);
        }
        if (alive.empty()) return false;
        // prefer low blocks: hitting the base topples whole stacks
        sort(alive.begin(), alive.end(), [](Block* a, Block* b) {
            return a->getGlobalPos().y < b->getGlobalPos().y;
        });
        int span = std::max(1, (int)alive.size() / 2);
        Block* target = alive[(int)random(0.0f, (float)span - 0.001f)];

        Vec3 base = cannon_->getGlobalPos() + Vec3(0, Cannon::PIVOT_H, 0);
        Vec3 tpos = target->getGlobalPos();
        float g = 12.0f;
        float yawSol = atan2f(-(tpos.x - base.x), -(tpos.z - base.z));

        // the CPU likes ~80% power: flat-ish, fast shots shove blocks off the
        // platform (slow lobs drop onto blocks from above and pin them down).
        // Never 0.93+: a MAX shot (and its loud sound) is the player's reward.
        for (float power : {0.8f, 0.88f, 0.7f, 0.92f, 0.6f}) {
            float v = Cannon::MIN_SPEED + power * (Cannon::MAX_SPEED - Cannon::MIN_SPEED);
            // two passes: solve from the pivot, then re-solve from the actual
            // muzzle (the ball spawns BARREL_LEN along the barrel — ignoring
            // that overshoots small targets by ~0.3 m at range)
            Vec3 from = base;
            float pitch = 0.0f;
            bool ok = false;
            for (int pass = 0; pass < 2; pass++) {
                Vec3 d3 = tpos - from;
                float dist = sqrtf(d3.x * d3.x + d3.z * d3.z);
                float h = d3.y;
                float disc = v * v * v * v - g * (g * dist * dist + 2.0f * h * v * v);
                if (disc < 0) { ok = false; break; }
                pitch = atanf((v * v - sqrtf(disc)) / (g * dist));
                ok = (pitch >= 0.0f && pitch <= TAU * 0.18f);
                if (!ok) break;
                Vec3 dir(-sinf(yawSol) * cosf(pitch), sinf(pitch), -cosf(yawSol) * cosf(pitch));
                from = base + dir * Cannon::BARREL_LEN;
            }
            if (!ok) continue;
            // sniper mode on the last stragglers: no aim noise
            float n = (blocksLeft_ <= 2) ? 0.0f : 1.0f;
            aiYaw_   = yawSol + n * random(-0.008f, 0.008f);
            aiPitch_ = pitch + n * random(-0.006f, 0.006f);
            aiPower_ = clamp(power + n * random(-0.02f, 0.02f), 0.0f, 0.92f);
            return true;
        }
        // no clean solution: lob it hard (capped below the MAX-shot zone)
        aiYaw_ = yawSol;
        aiPitch_ = deg2rad(35.0f);
        aiPower_ = 0.92f;
        return true;
    }

    // --- members ---------------------------------------------------------------
    EasyCam cam_;
    Light keyLight_, fillLight_;

    shared_ptr<Cannon>    cannon_;
    Mesh                  dotMesh_;   // trajectory guide dot
    shared_ptr<TowerRoot> towerRoot_;
    shared_ptr<Node>      ballsRoot_;
    EventListener         firedL_, spawnL_;
    bool                  editMode_ = false;
    vector<EventListener> blockL_;

    vector<LevelDef> levels_;
    size_t levelIdx_ = 0;
    Phase  phase_ = Phase::Title;
    int    score_ = 0;
    int    hiScore_ = 0;
    int    shots_ = 0;
    int    blocksTotal_ = 0;
    int    blocksLeft_ = 0;
    int    lastBonus_ = 0;
    float  settle_ = 0;
    float  demoReload_ = 0;
    bool   autopilot_ = false;

    std::map<int, bool> held_;   // explicit std:: (tc::map collides)

    bool  aiAiming_ = false;
    float aiTimer_ = 0, aiInterval_ = 2.0f;
    float aiYaw_ = 0, aiPitch_ = 0, aiPower_ = 0.7f;
};
