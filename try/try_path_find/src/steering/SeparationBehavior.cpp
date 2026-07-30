#include "steering/SeparationBehavior.h"
#include "simulation/Agent.h"
#include "simulation/World.h"
#include <vector>

void SeparationBehavior::apply(std::vector<Agent>& agents, const World& world, float dt) {
    auto& hash = const_cast<World&>(world).getSpatialHash();
    int count = (int)agents.size();

    // Compute forces into a separate buffer (parallel-safe)
    std::vector<Vec2> forces(count, Vec2(0, 0));

    const_cast<World&>(world).getThreadPool().parallelFor(count, [&](int i) {
        std::vector<int> neighbors; // thread-local
        neighbors.reserve(32);
        hash.query(agents[i].position, separationRadius, neighbors);

        Vec2 force(0, 0);
        for (int ni : neighbors) {
            if (ni == i) continue;
            Vec2 diff = agents[i].position - agents[ni].position;
            float dist = diff.length();
            if (dist < separationRadius && dist > 1e-6f) {
                force += diff.normalized() * (separationStrength / dist);
            }
        }
        forces[i] = force;
    });

    // Apply forces (sequential, but trivial)
    for (int i = 0; i < count; ++i) {
        agents[i].velocity += forces[i];
    }
}