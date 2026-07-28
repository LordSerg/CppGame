#pragma once

#include <vector>
#include <cstdint>

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float dot(const Vec2& o) const { return x * o.x + y * o.y; }
    float lengthSq() const { return x * x + y * y; }
    float length() const;
    Vec2 normalized() const;
    Vec2 perpendicular() const { return {-y, x}; }
};

Vec2 operator*(float s, const Vec2& v);

struct Agent {
    uint32_t id = 0;
    Vec2 position;
    Vec2 velocity;
    Vec2 direction;           // facing direction (normalized)
    float radius;
    float maxSpeed;
    bool selected = false;
    bool hasGoal = false;
    Vec2 goal;

    // Current path to follow (list of waypoints)
    std::vector<Vec2> path;
    int currentWaypoint = 0;

    // Formation offset (relative to group center target)
    Vec2 formationOffset;

    Agent();
    Agent(uint32_t id, Vec2 pos, float radius, float speed);
};