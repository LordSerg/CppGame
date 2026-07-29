#pragma once

#include "steering/ISteeringBehavior.h"
#include "simulation/Agent.h"
#include <vector>

struct Line {
    Vec2 point;
    Vec2 direction;
};

// Full Optimal Reciprocal Collision Avoidance (ORCA) 
// using 2D Linear Programming.
class ORCABehavior : public ISteeringBehavior {
public:
    void apply(std::vector<Agent>& agents, const World& world, float dt) override;
    std::string name() const override { return "ORCA"; }

    float timeHorizon = 2.0f;
    float timeHorizonObst = 2.0f;
    float maxNeighbors = 10;
    float neighborDist = 50.0f;

private:
    bool linearProgram1(const std::vector<Line>& lines, size_t lineNo, float radius, const Vec2& optVelocity, bool dirOpt, Vec2& result);
    size_t linearProgram2(const std::vector<Line>& lines, float radius, const Vec2& optVelocity, bool dirOpt, Vec2& result);
    void linearProgram3(const std::vector<Line>& lines, size_t numObstLines, size_t beginLine, float radius, Vec2& result);
};