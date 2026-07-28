#include "formation/SquareFormation.h"
#include <cmath>

std::vector<Vec2> SquareFormation::computeOffsets(int count) const {
    std::vector<Vec2> offsets;
    int side = (int)std::ceil(std::sqrt((float)count));
    float halfW = (side - 1) * spacing * 0.5f;
    float halfH = ((count - 1) / side) * spacing * 0.5f;

    for (int i = 0; i < count; ++i) {
        int col = i % side;
        int row = i / side;
        offsets.push_back({col * spacing - halfW, row * spacing - halfH});
    }
    return offsets;
}