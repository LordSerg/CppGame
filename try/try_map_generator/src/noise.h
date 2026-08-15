#pragma once

#include <cstdint>
#include <vector>

class PerlinNoise {
public:
    PerlinNoise(uint32_t seed = 0);
    void reseed(uint32_t seed);

    double noise(double x, double y) const;
    double octaveNoise(double x, double y, int octaves, double persistence = 0.5) const;

private:
    std::vector<int> p_;
    double fade(double t) const;
    double lerp(double t, double a, double b) const;
    double grad(int hash, double x, double y) const;
};