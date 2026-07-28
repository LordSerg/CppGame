#include "steering/SeparationBehavior.h"
#include "simulation/Agent.h"
#include "simulation/World.h"

void SeparationBehavior::apply(std::vector<Agent>& agents, const World& world, float dt) {
    auto& hash = const_cast<World&>(world).getSpatialHash();
    for (auto& agent : agents) {
        Vec2 separationForce(0, 0);
        auto neighbors = hash.query(agent.position, separationRadius);
        for (int ni : neighbors) {
            if (agents[ni].id == agent.id) continue;
            Vec2 diff = agent.position - agents[ni].position;
            float dist = diff.length();
            if (dist < separationRadius && dist > 1e-6f) {
                separationForce += diff.normalized() * (separationStrength / dist);
            }
        }
        agent.velocity += separationForce;
    }
}