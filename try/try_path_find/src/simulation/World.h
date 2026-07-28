#pragma once

#include "simulation/Agent.h"
#include "simulation/Barrier.h"
#include "spatial/SpatialHash.h"
#include <vector>
#include <memory>
#include <cstdint>

class IPathfinder;
class ISteeringBehavior;
class IFormation;

class World {
public:
    World();
    ~World();

    // Agent management
    Agent& addAgent(Vec2 position);
    void removeAgent(uint32_t id);
    std::vector<Agent>& getAgents() { return agents_; }
    const std::vector<Agent>& getAgents() const { return agents_; }

    // Barrier management
    Barrier& addBarrier(Vec2 center, Vec2 halfExtents);
    void removeBarrier(uint32_t id);
    std::vector<Barrier>& getBarriers() { return barriers_; }
    const std::vector<Barrier>& getBarriers() const { return barriers_; }

    // Selection
    void selectAgentsInRect(Vec2 min, Vec2 max);
    void clearSelection();
    std::vector<Agent*> getSelectedAgents();

    // Commanding
    void commandSelectedTo(Vec2 target);

    // Pathfinder
    void setPathfinder(std::shared_ptr<IPathfinder> pf);
    IPathfinder* getPathfinder() const { return pathfinder_.get(); }

    // Steering
    void setSteeringBehavior(std::shared_ptr<ISteeringBehavior> sb);
    ISteeringBehavior* getSteeringBehavior() const { return steering_.get(); }

    // Formation
    void setFormation(std::shared_ptr<IFormation> f);
    IFormation* getFormation() const { return formation_.get(); }

    // Update
    void update(float dt);

    // Spatial queries
    SpatialHash& getSpatialHash() { return spatialHash_; }

    // Check if a circle at position with radius overlaps any barrier
    bool collidesWithBarrier(Vec2 pos, float radius) const;

    // Check if a line segment from a to b intersects any barrier
    bool lineIntersectsBarrier(Vec2 a, Vec2 b) const;

private:
    void rebuildSpatialHash();
    void resolveAgentCollisions();

    std::vector<Agent> agents_;
    std::vector<Barrier> barriers_;
    uint32_t nextAgentId_ = 1;
    uint32_t nextBarrierId_ = 1;

    std::shared_ptr<IPathfinder> pathfinder_;
    std::shared_ptr<ISteeringBehavior> steering_;
    std::shared_ptr<IFormation> formation_;

    SpatialHash spatialHash_;
};