#include "rendering/Renderer.h"
#include "core/Config.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>

#include <cmath>

Renderer::Renderer() {}

void Renderer::beginFrame(float viewportW, float viewportH, const Camera& camera) {
    viewportW_ = viewportW;
    viewportH_ = viewportH;

    glViewport(0, 0, (int)viewportW, (int)viewportH);
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float left, right, bottom, top;
    camera.getViewBounds(viewportW, viewportH, left, right, bottom, top);
    glOrtho(left, right, top, bottom, -1.0, 1.0); // note: top/bottom flipped so Y goes down

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::endFrame() {
    glDisable(GL_BLEND);
}

void Renderer::drawFilledRect(Vec2 center, Vec2 halfExtents, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(center.x - halfExtents.x, center.y - halfExtents.y);
    glVertex2f(center.x + halfExtents.x, center.y - halfExtents.y);
    glVertex2f(center.x + halfExtents.x, center.y + halfExtents.y);
    glVertex2f(center.x - halfExtents.x, center.y + halfExtents.y);
    glEnd();
}

void Renderer::drawRect(Vec2 center, Vec2 halfExtents, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_LINE_LOOP);
    glVertex2f(center.x - halfExtents.x, center.y - halfExtents.y);
    glVertex2f(center.x + halfExtents.x, center.y - halfExtents.y);
    glVertex2f(center.x + halfExtents.x, center.y + halfExtents.y);
    glVertex2f(center.x - halfExtents.x, center.y + halfExtents.y);
    glEnd();
}

void Renderer::drawCircle(Vec2 center, float radius, float r, float g, float b, float a, bool filled) {
    glColor4f(r, g, b, a);
    glBegin(filled ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
    if (filled) glVertex2f(center.x, center.y);
    for (int i = 0; i <= Config::CIRCLE_SEGMENTS; ++i) {
        float angle = 2.0f * 3.14159f * i / Config::CIRCLE_SEGMENTS;
        glVertex2f(center.x + radius * std::cos(angle),
                    center.y + radius * std::sin(angle));
    }
    glEnd();
}

void Renderer::drawLine(Vec2 a, Vec2 b, float r, float g, float b2, float a2) {
    glColor4f(r, g, b2, a2);
    glBegin(GL_LINES);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glEnd();
}

void Renderer::drawTriangle(Vec2 a, Vec2 b, Vec2 c, float r, float g, float b2, float a2) {
    glColor4f(r, g, b2, a2);
    glBegin(GL_TRIANGLES);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glVertex2f(c.x, c.y);
    glEnd();
}

void Renderer::drawWorld(float mapW, float mapH) {
    // Map background
    drawFilledRect({mapW * 0.5f, mapH * 0.5f}, {mapW * 0.5f, mapH * 0.5f},
                   0.2f, 0.25f, 0.2f, 1.0f);
    // Map border
    drawRect({mapW * 0.5f, mapH * 0.5f}, {mapW * 0.5f, mapH * 0.5f},
             0.5f, 0.5f, 0.5f, 1.0f);
}

void Renderer::drawBarriers(const std::vector<Barrier>& barriers) {
    for (const auto& b : barriers) {
        drawFilledRect(b.center, b.halfExtents, 0.6f, 0.3f, 0.2f, 0.9f);
        drawRect(b.center, b.halfExtents, 0.8f, 0.4f, 0.3f, 1.0f);
    }
}

void Renderer::drawAgents(const std::vector<Agent>& agents) {
    for (const auto& a : agents) {
        if (a.selected) {
            // Selection ring
            drawCircle(a.position, a.radius + 2.0f, 0.2f, 0.8f, 0.2f, 0.7f, false);
        }
        // Agent body
        drawCircle(a.position, a.radius, 0.3f, 0.5f, 0.9f, 0.9f, true);
        // Direction indicator
        Vec2 tip = a.position + a.direction * a.radius * 1.5f;
        drawLine(a.position, tip, 1.0f, 1.0f, 1.0f, 0.8f);
    }
}

void Renderer::drawPaths(const std::vector<Agent>& agents) {
    for (const auto& a : agents) {
        if (a.path.empty() || !a.hasGoal) continue;

        // Draw from agent to first waypoint
        Vec2 prev = a.position;
        for (int i = a.currentWaypoint; i < (int)a.path.size(); ++i) {
            drawLine(prev, a.path[i], 0.8f, 0.8f, 0.2f, 0.5f);
            prev = a.path[i];
        }
        // Goal marker
        drawCircle(a.goal, 3.0f, 1.0f, 0.3f, 0.3f, 0.6f, false);
    }
}

void Renderer::drawSelectionRect(Vec2 start, Vec2 end) {
    Vec2 center = {(start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f};
    Vec2 half = {std::abs(end.x - start.x) * 0.5f, std::abs(end.y - start.y) * 0.5f};
    drawFilledRect(center, half, 0.2f, 0.8f, 0.2f, 0.15f);
    drawRect(center, half, 0.2f, 0.8f, 0.2f, 0.6f);
}

void Renderer::drawGrid(const Grid& grid) {
    for (int r = 0; r < grid.getRows(); ++r) {
        for (int c = 0; c < grid.getCols(); ++c) {
            float x, y;
            Grid::gridToWorld(c, r, x, y);
            Vec2 halfCell(Config::CELL_WIDTH * 0.5f, Config::CELL_HEIGHT * 0.5f);
            if (grid.isBlocked(c, r)) {
                drawFilledRect({x, y}, halfCell, 0.5f, 0.2f, 0.2f, 0.3f);
            }
            drawRect({x, y}, halfCell, 0.3f, 0.3f, 0.3f, 0.15f);
        }
    }
}

void Renderer::drawNavMesh(const NavMesh& navMesh) {
    for (const auto& tri : navMesh.getTriangles()) {
        drawTriangle(tri.vertices[0], tri.vertices[1], tri.vertices[2],
                     0.2f, 0.4f, 0.6f, 0.1f);
        drawLine(tri.vertices[0], tri.vertices[1], 0.3f, 0.5f, 0.7f, 0.3f);
        drawLine(tri.vertices[1], tri.vertices[2], 0.3f, 0.5f, 0.7f, 0.3f);
        drawLine(tri.vertices[2], tri.vertices[0], 0.3f, 0.5f, 0.7f, 0.3f);
    }
}