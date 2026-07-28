#include "formation/WedgeFormation.h"
#include <cmath>

std::vector<Vec2> WedgeFormation::computeOffsets(int count) const {
    std::vector<Vec2> offsets;
    // V-shape: leader at front, then alternating left/right
    offsets.push_back({0, 0}); // leader
    for (int i = 1; i < count; ++i) {
        int row = (i + 1) / 2;
        float side = (i % 2 == 1) ? -1.0f : 1.0f;
        offsets.push_back({side * row * spacing, row * spacing});
    }
    return offsets;
}