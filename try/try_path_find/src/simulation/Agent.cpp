#include "simulation/Agent.h"
#include "core/Config.h"
#include <cmath>

float Vec2::length() const {
    return std::sqrt(x * x + y * y);
}

Vec2 Vec2::normalized() const {
    float len = length();
    if (len < 1e-8f) return {0.0f, 0.0f};
    return {x / len, y / len};
}

Vec2 operator*(float s, const Vec2& v) {
    return {v.x * s, v.y * s};
}

Agent::Agent()
    : id(0), radius(Config::DEFAULT_AGENT_RADIUS),
      maxSpeed(Config::DEFAULT_AGENT_SPEED), direction(0.0f, 1.0f) {}

Agent::Agent(uint32_t id, Vec2 pos, float radius, float speed)
    : id(id), position(pos), radius(radius), maxSpeed(speed),
      direction(0.0f, 1.0f) {}