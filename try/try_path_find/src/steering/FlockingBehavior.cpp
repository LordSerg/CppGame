#include "steering/FlockingBehavior.h"
#include "simulation/Agent.h"
#include "simulation/World.h"

void FlockingBehavior::apply(std::vector<Agent>& agents, const World& world, float dt) {
    auto& hash = const_cast<World&>(world).getSpatialHash();
    std::vector<int> neighbors;
    neighbors.reserve(32);

    for (size_t i = 0; i < agents.size(); ++i) {
        auto& agent = agents[i];
        Vec2 separation(0, 0), alignment(0, 0), cohesion(0, 0);
        int count = 0;

        hash.query(agent.position, neighborRadius, neighbors);
        for (int ni : neighbors) {
            if (ni == (int)i) continue;
            auto& other = agents[ni];

            Vec2 diff = agent.position - other.position;
            float dist = diff.length();
            if (dist < 1e-6f || dist > neighborRadius) continue;

            separation += diff.normalized() * (1.0f / dist);
            alignment += other.velocity;
            cohesion += other.position;
            count++;
        }

        if (count > 0) {
            float inv = 1.0f / count;
            alignment = alignment * inv;
            cohesion = (cohesion * inv) - agent.position;

            agent.velocity += separation * separationWeight
                            + (alignment - agent.velocity) * alignmentWeight
                            + cohesion * cohesionWeight;
        }
    }
}