#include "formation/CircleFormation.h"
#include <cmath>

std::vector<Vec2> CircleFormation::computeOffsets(int count) const {
    std::vector<Vec2> offsets;
    if (count <= 1) {
        offsets.push_back({0, 0});
        return offsets;
    }
    float radius = spacing * count / (2.0f * 3.14159f);
    for (int i = 0; i < count; ++i) {
        float angle = 2.0f * 3.14159f * i / count;
        offsets.push_back({radius * std::cos(angle), radius * std::sin(angle)});
    }
    return offsets;
}