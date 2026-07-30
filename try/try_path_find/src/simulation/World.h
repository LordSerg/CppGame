#pragma once

#include "simulation/Agent.h"
#include "simulation/Barrier.h"
#include "spatial/SpatialHash.h"
#include "core/ThreadPool.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <queue>
#include <mutex>

class IPathfinder;
class ISteeringBehavior;
class IFormation;

class World {
public:
    World();
    ~World();

    Agent& addAgent(Vec2 position);
    void removeAgent(uint32_t id);
    std::vector<Agent>& getAgents() { return agents_; }
    const std::vector<Agent>& getAgents() const { return agents_; }

    Barrier& addBarrier(Vec2 center, Vec2 halfExtents);
    void removeBarrier(uint32_t id);
    std::vector<Barrier>& getBarriers() { return barriers_; }
    const std::vector<Barrier>& getBarriers() const { return barriers_; }

    void selectAgentsInRect(Vec2 min, Vec2 max);
    void clearSelection();
    std::vector<Agent*> getSelectedAgents();

    void commandSelectedTo(Vec2 target);

    void setPathfinder(std::shared_ptr<IPathfinder> pf);
    IPathfinder* getPathfinder() const { return pathfinder_.get(); }

    void setSteeringBehavior(std::shared_ptr<ISteeringBehavior> sb);
    ISteeringBehavior* getSteeringBehavior() const { return steering_.get(); }

    void setFormation(std::shared_ptr<IFormation> f);
    IFormation* getFormation() const { return formation_.get(); }

    void update(float dt);

    SpatialHash& getSpatialHash() { return spatialHash_; }

    bool collidesWithBarrier(Vec2 pos, float radius) const;
    bool lineIntersectsBarrier(Vec2 a, Vec2 b) const;

    int pendingPathRequests() const { return (int)pathQueue_.size(); }

    ThreadPool& getThreadPool() { return threadPool_; }

private:
    void rebuildSpatialHash();
    void resolveAgentCollisions();
    void processPathQueue();

    std::vector<Agent> agents_;
    std::vector<Barrier> barriers_;
    uint32_t nextAgentId_ = 1;
    uint32_t nextBarrierId_ = 1;

    std::shared_ptr<IPathfinder> pathfinder_;
    std::shared_ptr<ISteeringBehavior> steering_;
    std::shared_ptr<IFormation> formation_;

    SpatialHash spatialHash_;

    struct PathRequest {
        uint32_t agentId;
        Vec2 target;
    };
    std::queue<PathRequest> pathQueue_;

    std::vector<int> neighborBuffer_;

    ThreadPool threadPool_;
};