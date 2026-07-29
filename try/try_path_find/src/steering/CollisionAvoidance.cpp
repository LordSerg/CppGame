#include "steering/CollisionAvoidance.h"
#include "simulation/Agent.h"
#include "simulation/World.h"
#include <cmath>

void CollisionAvoidance::apply(std::vector<Agent>& agents, const World& world, float dt) {
    auto& hash = const_cast<World&>(world).getSpatialHash();
    float maxRange = 60.0f;
    std::vector<int> neighbors;
    neighbors.reserve(32);

    for (size_t i = 0; i < agents.size(); ++i) {
        auto& agent = agents[i];
        Vec2 avoidForce(0, 0);
        hash.query(agent.position, maxRange, neighbors);

        float closestTime = lookAheadTime;
        Vec2 closestAvoid(0, 0);

        for (int ni : neighbors) {
            if (ni == (int)i) continue;
            auto& other = agents[ni];

            Vec2 relPos = other.position - agent.position;
            Vec2 relVel = agent.velocity - other.velocity;
            float relSpeed = relVel.length();
            if (relSpeed < 1e-6f) continue;

            float t = relPos.dot(relVel) / (relSpeed * relSpeed);
            t = std::max(0.0f, std::min(t, lookAheadTime));

            Vec2 futureA = agent.position + agent.velocity * t;
            Vec2 futureB = other.position + other.velocity * t;
            Vec2 diff = futureA - futureB;
            float dist = diff.length();
            float minSep = agent.radius + other.radius + 2.0f;

            if (dist < minSep && t < closestTime) {
                closestTime = t;
                if (dist > 1e-6f)
                    closestAvoid = diff.normalized() * avoidanceStrength * (1.0f - t / lookAheadTime);
                else
                    closestAvoid = agent.direction.perpendicular() * avoidanceStrength;
            }
        }

        agent.velocity += closestAvoid;
    }
}