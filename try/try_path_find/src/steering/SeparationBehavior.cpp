#include "steering/SeparationBehavior.h"
#include "simulation/Agent.h"
#include "simulation/World.h"

void SeparationBehavior::apply(std::vector<Agent>& agents, const World& world, float dt) {
    auto& hash = const_cast<World&>(world).getSpatialHash();
    std::vector<int> neighbors; // reused buffer
    neighbors.reserve(32);

    for (size_t i = 0; i < agents.size(); ++i) {
        auto& agent = agents[i];
        Vec2 separationForce(0, 0);

        hash.query(agent.position, separationRadius, neighbors);
        for (int ni : neighbors) {
            if (ni == (int)i) continue;
            Vec2 diff = agent.position - agents[ni].position;
            float dist = diff.length();
            if (dist < separationRadius && dist > 1e-6f) {
                separationForce += diff.normalized() * (separationStrength / dist);
            }
        }
        agent.velocity += separationForce;
    }
}