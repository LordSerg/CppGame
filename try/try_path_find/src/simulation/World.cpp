#include "simulation/World.h"
#include "pathfinding/IPathfinder.h"
#include "steering/ISteeringBehavior.h"
#include "formation/IFormation.h"
#include "core/Config.h"
#include <algorithm>
#include <cmath>
#include <future>

World::World() : spatialHash_(Config::SPATIAL_HASH_CELL) {
    neighborBuffer_.reserve(64);
}

World::~World() = default;

Agent& World::addAgent(Vec2 position) {
    agents_.emplace_back(nextAgentId_++, position,
                         Config::DEFAULT_AGENT_RADIUS,
                         Config::DEFAULT_AGENT_SPEED);
    return agents_.back();
}

void World::removeAgent(uint32_t id) {
    agents_.erase(
        std::remove_if(agents_.begin(), agents_.end(),
                       [id](const Agent& a) { return a.id == id; }),
        agents_.end());
}

Barrier& World::addBarrier(Vec2 center, Vec2 halfExtents) {
    barriers_.emplace_back(nextBarrierId_++, center, halfExtents);
    return barriers_.back();
}

void World::removeBarrier(uint32_t id) {
    barriers_.erase(
        std::remove_if(barriers_.begin(), barriers_.end(),
                       [id](const Barrier& b) { return b.id == id; }),
        barriers_.end());
}

void World::selectAgentsInRect(Vec2 min, Vec2 max) {
    for (auto& a : agents_) {
        a.selected = (a.position.x >= min.x && a.position.x <= max.x &&
                      a.position.y >= min.y && a.position.y <= max.y);
    }
}

void World::clearSelection() {
    for (auto& a : agents_) a.selected = false;
}

std::vector<Agent*> World::getSelectedAgents() {
    std::vector<Agent*> sel;
    for (auto& a : agents_) {
        if (a.selected) sel.push_back(&a);
    }
    return sel;
}

void World::commandSelectedTo(Vec2 target) {
    auto selected = getSelectedAgents();
    if (selected.empty()) return;

    std::vector<Vec2> offsets;
    if (formation_) {
        offsets = formation_->computeOffsets(static_cast<int>(selected.size()));
    }

    for (size_t i = 0; i < selected.size(); ++i) {
        Vec2 offset = (i < offsets.size()) ? offsets[i] : Vec2(0, 0);
        selected[i]->formationOffset = offset;
        Vec2 agentTarget = target + offset;
        selected[i]->goal = agentTarget;
        selected[i]->hasGoal = true;
        selected[i]->currentWaypoint = 0;
        selected[i]->path.clear();

        pathQueue_.push({selected[i]->id, agentTarget});
    }
}

void World::processPathQueue() {
    if (pathQueue_.empty() || !pathfinder_) return;

    // Collect a batch of requests
    std::vector<PathRequest> batch;
    int maxBatch = Config::MAX_PATHS_PER_FRAME;
    while (!pathQueue_.empty() && (int)batch.size() < maxBatch) {
        batch.push_back(pathQueue_.front());
        pathQueue_.pop();
    }

    if (batch.empty()) return;

    // Map agent IDs to indices for fast lookup
    struct BatchResult {
        int agentIdx;
        std::vector<Vec2> path;
    };
    std::vector<BatchResult> results(batch.size());

    // Find agent indices
    for (size_t b = 0; b < batch.size(); ++b) {
        results[b].agentIdx = -1;
        for (size_t a = 0; a < agents_.size(); ++a) {
            if (agents_[a].id == batch[b].agentId) {
                results[b].agentIdx = (int)a;
                break;
            }
        }
    }

    // Parallel pathfinding: each search uses thread-local storage
    // The grid is read-only, the World (for lineIntersectsBarrier) is read-only,
    // and each result writes to its own slot.
    const World* worldPtr = this;
    IPathfinder* pf = pathfinder_.get();

    threadPool_.parallelFor((int)batch.size(), [&](int b) {
        if (results[b].agentIdx < 0) return;
        Agent& agent = agents_[results[b].agentIdx];
        if (!agent.hasGoal) return;
        results[b].path = pf->findPath(agent.position, batch[b].target, *worldPtr);
    });

    // Apply results (single-threaded, modifies agents)
    for (size_t b = 0; b < batch.size(); ++b) {
        if (results[b].agentIdx < 0) continue;
        Agent& agent = agents_[results[b].agentIdx];
        if (!agent.hasGoal) continue;
        agent.path = std::move(results[b].path);
        agent.currentWaypoint = 0;
    }
}

void World::setPathfinder(std::shared_ptr<IPathfinder> pf) { pathfinder_ = pf; }
void World::setSteeringBehavior(std::shared_ptr<ISteeringBehavior> sb) { steering_ = sb; }
void World::setFormation(std::shared_ptr<IFormation> f) { formation_ = f; }

bool World::collidesWithBarrier(Vec2 pos, float radius) const {
    for (const auto& b : barriers_) {
        Vec2 closest = b.closestPoint(pos);
        Vec2 diff = pos - closest;
        if (diff.lengthSq() < radius * radius) return true;
    }
    return false;
}

bool World::lineIntersectsBarrier(Vec2 a, Vec2 b) const {
    for (const auto& bar : barriers_) {
        Vec2 bmin = bar.center - bar.halfExtents;
        Vec2 bmax = bar.center + bar.halfExtents;
        float ddx = b.x - a.x;
        float ddy = b.y - a.y;
        float p[4] = {-ddx, ddx, -ddy, ddy};
        float q[4] = {a.x - bmin.x, bmax.x - a.x, a.y - bmin.y, bmax.y - a.y};
        float tmin = 0.0f, tmax = 1.0f;
        bool outside = false;
        for (int i = 0; i < 4; ++i) {
            if (std::abs(p[i]) < 1e-10f) {
                if (q[i] < 0) { outside = true; break; }
            } else {
                float t = q[i] / p[i];
                if (p[i] < 0) { if (t > tmin) tmin = t; }
                else { if (t < tmax) tmax = t; }
            }
        }
        if (!outside && tmin <= tmax) return true;
    }
    return false;
}

void World::rebuildSpatialHash() {
    spatialHash_.clear();
    spatialHash_.reserve((int)agents_.size());
    for (size_t i = 0; i < agents_.size(); ++i) {
        spatialHash_.insert(static_cast<int>(i), agents_[i].position);
    }
}

void World::resolveAgentCollisions() {
    const int iterations = 3;
    for (int iter = 0; iter < iterations; ++iter) {
        if (iter > 0) rebuildSpatialHash();

        for (size_t i = 0; i < agents_.size(); ++i) {
            spatialHash_.query(agents_[i].position, agents_[i].radius * 2.5f, neighborBuffer_);
            for (int j : neighborBuffer_) {
                if (j <= (int)i) continue;
                Vec2 diff = agents_[i].position - agents_[j].position;
                float distSq = diff.lengthSq();
                float minSep = agents_[i].radius + agents_[j].radius;
                if (distSq < minSep * minSep && distSq > 1e-8f) {
                    float dist = std::sqrt(distSq);
                    Vec2 push = diff * (1.0f / dist);
                    float penetration = minSep - dist;
                    agents_[i].position += push * (penetration * 0.5f);
                    agents_[j].position -= push * (penetration * 0.5f);
                }
            }
        }
    }
}

void World::update(float dt) {
    processPathQueue();
    rebuildSpatialHash();

    // Compute desired velocities (parallel - each agent independent)
    threadPool_.parallelFor((int)agents_.size(), [this](int i) {
        Agent& agent = agents_[i];
        if (!agent.hasGoal || agent.path.empty()) {
            agent.velocity = Vec2(0, 0);
            return;
        }
        if (agent.currentWaypoint >= (int)agent.path.size()) {
            agent.hasGoal = false;
            agent.velocity = Vec2(0, 0);
            return;
        }

        Vec2 target = agent.path[agent.currentWaypoint];
        Vec2 toTarget = target - agent.position;
        float dist = toTarget.length();

        if (dist < Config::WAYPOINT_REACH_DIST) {
            agent.currentWaypoint++;
            if (agent.currentWaypoint >= (int)agent.path.size()) {
                agent.hasGoal = false;
                agent.velocity = Vec2(0, 0);
                return;
            }
            target = agent.path[agent.currentWaypoint];
            toTarget = target - agent.position;
            dist = toTarget.length();
        }

        Vec2 desired = toTarget.normalized() * agent.maxSpeed;
        if (agent.currentWaypoint == (int)agent.path.size() - 1 &&
            dist < Config::ARRIVAL_SLOW_RADIUS) {
            desired = desired * (dist / Config::ARRIVAL_SLOW_RADIUS);
        }
        agent.velocity = desired;
    });

    // Apply steering behaviors
    if (steering_) {
        steering_->apply(agents_, *this, dt);
    }

    // Integrate positions (parallel)
    const float mapW = Config::MAP_WIDTH;
    const float mapH = Config::MAP_HEIGHT;
    threadPool_.parallelFor((int)agents_.size(), [this, dt, mapW, mapH](int i) {
        Agent& agent = agents_[i];
        if (agent.velocity.lengthSq() > agent.maxSpeed * agent.maxSpeed) {
            agent.velocity = agent.velocity.normalized() * agent.maxSpeed;
        }
        agent.position += agent.velocity * dt;
        if (agent.velocity.lengthSq() > 1e-4f) {
            agent.direction = agent.velocity.normalized();
        }
        agent.position.x = std::max(agent.radius,
                           std::min(agent.position.x, mapW - agent.radius));
        agent.position.y = std::max(agent.radius,
                           std::min(agent.position.y, mapH - agent.radius));
    });

    // Push agents out of barriers (parallel - each agent vs all barriers is independent)
    threadPool_.parallelFor((int)agents_.size(), [this](int i) {
        Agent& agent = agents_[i];
        for (const auto& b : barriers_) {
            Vec2 closest = b.closestPoint(agent.position);
            Vec2 diff = agent.position - closest;
            float dist = diff.length();
            if (dist < agent.radius && dist > 1e-6f) {
                agent.position += diff.normalized() * (agent.radius - dist);
            } else if (dist < 1e-6f && b.contains(agent.position)) {
                float dx1 = agent.position.x - (b.center.x - b.halfExtents.x);
                float dx2 = (b.center.x + b.halfExtents.x) - agent.position.x;
                float dy1 = agent.position.y - (b.center.y - b.halfExtents.y);
                float dy2 = (b.center.y + b.halfExtents.y) - agent.position.y;
                float minD = std::min({dx1, dx2, dy1, dy2});
                if (minD == dx1) agent.position.x -= dx1 + agent.radius;
                else if (minD == dx2) agent.position.x += dx2 + agent.radius;
                else if (minD == dy1) agent.position.y -= dy1 + agent.radius;
                else agent.position.y += dy2 + agent.radius;
            }
        }
    });

    // Resolve agent-agent collisions (sequential - agents affect each other)
    rebuildSpatialHash();
    resolveAgentCollisions();
}