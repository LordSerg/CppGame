#pragma once
#include "formation/IFormation.h"

class SquareFormation : public IFormation {
public:
    std::vector<Vec2> computeOffsets(int count) const override;
    std::string name() const override { return "Square"; }
};