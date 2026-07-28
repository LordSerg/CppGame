#pragma once
#include "formation/IFormation.h"

class CircleFormation : public IFormation {
public:
    std::vector<Vec2> computeOffsets(int count) const override;
    std::string name() const override { return "Circle"; }
};