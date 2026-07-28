#pragma once

#include "pathfinding/IPathfinder.h"
#include "pathfinding/Grid.h"
#include <vector>

// Flow field: computes a direction for every cell toward the target.
// Good for many agents going to the same destination.
class FlowFieldPathfinder : public IPathfinder {
public:
    std::vector<Vec2> findPath(Vec2 start, Vec2 end, const World& world) override;
    std::string name() const override { return "Flow Field"; }
    void onWorldChanged(const World& world) override { gridDirty_ = true; }

    // Access the flow field for visualization
    const std::vector<Vec2>& getFlowField() const { return flowField_; }
    bool hasFlowField() const { return !flowField_.empty(); }

private:
    void buildField(int targetCol, int targetRow);

    Grid grid_;
    bool gridDirty_ = true;
    std::vector<float> costField_;
    std::vector<Vec2> flowField_;
    int lastTargetCol_ = -1, lastTargetRow_ = -1;
};