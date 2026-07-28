#include "steering/FlockingBehavior.h"
#include "simulation/Agent.h"
#include "simulation/World.h"

void FlockingBehavior::apply(std::vector<Agent>& agents, const World& world, float dt) {
    auto& hash = const_cast<World&>(world).getSpatialHash();

    for (auto& agent : agents) {
        Vec2 separation(0, 0), alignment(0, 0), cohesion(0, 0);
        int count = 0;

        auto neighbors = hash.query(agent.position, neighborRadius);
        for (int ni : neighbors) {
            auto& other = agents[ni];
            if (other.id == agent.id) continue;

            Vec2 diff = agent.position - other.position;
            float dist = diff.length();
            if (dist < 1e-6f || dist > neighborRadius) continue;

            // Separation
            separation += diff.normalized() * (1.0f / dist);
            // Alignment
            alignment += other.velocity;
            // Cohesion
            cohesion += other.position;
            count++;
        }

        if (count > 0) {
            alignment = alignment * (1.0f / count);
            cohesion = (cohesion * (1.0f / count)) - agent.position;

            agent.velocity += separation * separationWeight
                            + (alignment - agent.velocity) * alignmentWeight
                            + cohesion * cohesionWeight;
        }
    }
}