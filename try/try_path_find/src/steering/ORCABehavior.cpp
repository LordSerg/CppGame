#include "steering/ORCABehavior.h"
#include "simulation/Agent.h"
#include "simulation/World.h"
#include <cmath>

void ORCABehavior::apply(std::vector<Agent>& agents, const World& world, float dt) {
    auto& hash = const_cast<World&>(world).getSpatialHash();
    float neighborRange = 50.0f;

    for (auto& agent : agents) {
        Vec2 totalAdjust(0, 0);
        int count = 0;

        auto neighbors = hash.query(agent.position, neighborRange);
        for (int ni : neighbors) {
            auto& other = agents[ni];
            if (other.id == agent.id) continue;

            Vec2 relPos = other.position - agent.position;
            Vec2 relVel = agent.velocity - other.velocity;
            float dist = relPos.length();
            float combinedRadius = agent.radius + other.radius;

            if (dist < 1e-6f) continue;

            // Compute the velocity obstacle boundary
            float invTau = 1.0f / timeHorizon;
            Vec2 u; // correction velocity

            if (dist > combinedRadius) {
                // No collision yet
                Vec2 w = relVel - relPos * invTau;
                float wLen = w.length();
                if (wLen < 1e-6f) continue;

                Vec2 wNorm = w.normalized();
                // Check if w is in the collision cone
                float dot = w.dot(relPos.normalized());
                if (dot < 0 && relPos.lengthSq() > combinedRadius * combinedRadius) {
                    continue; // diverging
                }

                u = wNorm * (combinedRadius * invTau - wLen);
            } else {
                // Already overlapping, push apart
                Vec2 dir = relPos.normalized();
                u = dir * (-agent.maxSpeed * 0.5f);
            }

            totalAdjust += u * 0.5f; // each agent takes half responsibility
            count++;
        }

        if (count > 0) {
            agent.velocity += totalAdjust;
        }
    }
}