#pragma once

#include "simulation/Agent.h"
#include "simulation/Barrier.h"
#include "camera/Camera.h"
#include "pathfinding/Grid.h"
#include "pathfinding/NavMesh.h"
#include <vector>

class Renderer {
public:
    Renderer();

    void beginFrame(float viewportW, float viewportH, const Camera& camera);
    void endFrame();

    // Drawing primitives (in world coordinates; transformed by camera in beginFrame)
    void drawFilledRect(Vec2 center, Vec2 halfExtents, float r, float g, float b, float a = 1.0f);
    void drawRect(Vec2 center, Vec2 halfExtents, float r, float g, float b, float a = 1.0f);
    void drawCircle(Vec2 center, float radius, float r, float g, float b, float a = 1.0f, bool filled = true);
    void drawLine(Vec2 a, Vec2 b, float r, float g, float b2, float a2 = 1.0f);
    void drawTriangle(Vec2 a, Vec2 b, Vec2 c, float r, float g, float b2, float a2 = 0.3f);

    // High-level drawing
    void drawWorld(float mapW, float mapH);
    void drawBarriers(const std::vector<Barrier>& barriers);
    void drawAgents(const std::vector<Agent>& agents);
    void drawPaths(const std::vector<Agent>& agents);
    void drawSelectionRect(Vec2 start, Vec2 end);
    void drawGrid(const Grid& grid);
    void drawNavMesh(const NavMesh& navMesh);

private:
    float viewportW_, viewportH_;
};