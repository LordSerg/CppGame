#include "formation/LineFormation.h"

std::vector<Vec2> LineFormation::computeOffsets(int count) const {
    std::vector<Vec2> offsets;
    float halfWidth = (count - 1) * spacing * 0.5f;
    for (int i = 0; i < count; ++i) {
        offsets.push_back({i * spacing - halfWidth, 0.0f});
    }
    return offsets;
}