#include "steering/ORCABehavior.h"
#include "simulation/Agent.h"
#include "simulation/World.h"
#include <cmath>
#include <algorithm>

const float RVO_EPSILON = 0.00001f;

static float det(const Vec2& v1, const Vec2& v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

bool ORCABehavior::linearProgram1(const std::vector<Line>& lines, size_t lineNo, float radius, const Vec2& optVelocity, bool dirOpt, Vec2& result) {
    float dotProduct = lines[lineNo].point.dot(lines[lineNo].direction);
    float discriminant = dotProduct * dotProduct + radius * radius - lines[lineNo].point.lengthSq();

    if (discriminant < 0.0f) return false;

    float sqrtDiscriminant = std::sqrt(discriminant);
    float tLeft = -dotProduct - sqrtDiscriminant;
    float tRight = -dotProduct + sqrtDiscriminant;

    for (size_t i = 0; i < lineNo; ++i) {
        float denominator = det(lines[lineNo].direction, lines[i].direction);
        float numerator = det(lines[i].direction, lines[lineNo].point - lines[i].point);

        if (std::abs(denominator) <= RVO_EPSILON) {
            if (numerator < 0.0f) return false;
            continue;
        }

        float t = numerator / denominator;
        if (denominator >= 0.0f) tRight = std::min(tRight, t);
        else tLeft = std::max(tLeft, t);

        if (tLeft > tRight) return false;
    }

    if (dirOpt) {
        result = lines[lineNo].point + ((optVelocity.dot(lines[lineNo].direction) > 0.0f) ? tRight : tLeft) * lines[lineNo].direction;
    } else {
        float t = lines[lineNo].direction.dot(optVelocity - lines[lineNo].point);
        t = std::max(tLeft, std::min(t, tRight));
        result = lines[lineNo].point + t * lines[lineNo].direction;
    }
    return true;
}

size_t ORCABehavior::linearProgram2(const std::vector<Line>& lines, float radius, const Vec2& optVelocity, bool dirOpt, Vec2& result) {
    if (dirOpt) result = optVelocity * radius;
    else if (optVelocity.lengthSq() > radius * radius) result = optVelocity.normalized() * radius;
    else result = optVelocity;

    for (size_t i = 0; i < lines.size(); ++i) {
        if (det(lines[i].direction, lines[i].point - result) > 0.0f) {
            Vec2 tempResult = result;
            if (!linearProgram1(lines, i, radius, optVelocity, dirOpt, result)) {
                result = tempResult;
                return i;
            }
        }
    }
    return lines.size();
}

void ORCABehavior::linearProgram3(const std::vector<Line>& lines, size_t numObstLines, size_t beginLine, float radius, Vec2& result) {
    float distance = 0.0f;
    for (size_t i = beginLine; i < lines.size(); ++i) {
        if (det(lines[i].direction, lines[i].point - result) > distance) {
            std::vector<Line> projLines(lines.begin(), lines.begin() + numObstLines);
            for (size_t j = numObstLines; j < i; ++j) {
                Line line;
                float determinant = det(lines[i].direction, lines[j].direction);
                if (std::abs(determinant) <= RVO_EPSILON) {
                    if (lines[i].direction.dot(lines[j].direction) > 0.0f) continue;
                    line.point = 0.5f * (lines[i].point + lines[j].point);
                } else {
                    line.point = lines[i].point + (det(lines[j].direction, lines[i].point - lines[j].point) / determinant) * lines[i].direction;
                }
                line.direction = (lines[j].direction - lines[i].direction).normalized();
                projLines.push_back(line);
            }
            Vec2 tempResult = result;
            if (linearProgram2(projLines, radius, Vec2(-lines[i].direction.y, lines[i].direction.x), true, result) < projLines.size()) {
                result = tempResult;
            }
            distance = det(lines[i].direction, lines[i].point - result);
        }
    }
}

void ORCABehavior::apply(std::vector<Agent>& agents, const World& world, float dt) {
    auto& hash = const_cast<World&>(world).getSpatialHash();
    std::vector<Vec2> newVelocities(agents.size());
    std::vector<int> neighbors;
    neighbors.reserve(32);

    for (size_t i = 0; i < agents.size(); ++i) {
        Agent& agent = agents[i];
        std::vector<Line> orcaLines;
        hash.query(agent.position, neighborDist, neighbors);

        float invTimeHorizon = 1.0f / timeHorizon;

        for (int ni : neighbors) {
            if (ni == (int)i) continue;
            Agent& other = agents[ni];
            Vec2 relPos = other.position - agent.position;
            Vec2 relVel = agent.velocity - other.velocity;
            float distSq = relPos.lengthSq();
            float combinedRadius = agent.radius + other.radius;
            float combinedRadiusSq = combinedRadius * combinedRadius;

            Line line;
            Vec2 u;

            if (distSq > combinedRadiusSq) {
                Vec2 w = relVel - relPos * invTimeHorizon;
                float wLengthSq = w.lengthSq();
                float dotProduct1 = w.dot(relPos);

                if (dotProduct1 < 0.0f && dotProduct1 * dotProduct1 > combinedRadiusSq * wLengthSq) {
                    float wLength = std::sqrt(wLengthSq);
                    Vec2 unitW = w * (1.0f / wLength);
                    line.direction = Vec2(unitW.y, -unitW.x);
                    u = (combinedRadius * invTimeHorizon - wLength) * unitW;
                } else {
                    float leg = std::sqrt(distSq - combinedRadiusSq);
                    if (det(relPos, w) > 0.0f) {
                        line.direction = Vec2(relPos.x * leg - relPos.y * combinedRadius,
                                              relPos.x * combinedRadius + relPos.y * leg) * (1.0f / distSq);
                    } else {
                        line.direction = Vec2(relPos.x * leg + relPos.y * combinedRadius,
                                               -relPos.x * combinedRadius + relPos.y * leg) * (-1.0f / distSq);
                    }
                    float dotProduct2 = relVel.dot(line.direction);
                    u = dotProduct2 * line.direction - relVel;
                }
            } else {
                float invTimeStep = 1.0f / dt;
                Vec2 w = relVel - relPos * invTimeStep;
                float wLength = w.length();
                if (wLength < RVO_EPSILON) continue;
                Vec2 unitW = w * (1.0f / wLength);
                line.direction = Vec2(unitW.y, -unitW.x);
                u = (combinedRadius * invTimeStep - wLength) * unitW;
            }

            line.point = agent.velocity + 0.5f * u;
            orcaLines.push_back(line);
        }

        size_t lineFail = linearProgram2(orcaLines, agent.maxSpeed, agent.velocity, false, newVelocities[i]);
        if (lineFail < orcaLines.size()) {
            linearProgram3(orcaLines, 0, lineFail, agent.maxSpeed, newVelocities[i]);
        }
    }

    for (size_t i = 0; i < agents.size(); ++i) {
        agents[i].velocity = newVelocities[i];
    }
}