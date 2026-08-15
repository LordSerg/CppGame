#include "noise.h"
#include <algorithm>
#include <numeric>
#include <random>
#include <cmath>

PerlinNoise::PerlinNoise(uint32_t seed) {
    reseed(seed);
}

void PerlinNoise::reseed(uint32_t seed) {
    p_.resize(512);
    std::vector<int> perm(256);
    std::iota(perm.begin(), perm.end(), 0);
    std::mt19937 gen(seed);
    std::shuffle(perm.begin(), perm.end(), gen);
    for (int i = 0; i < 256; i++) {
        p_[i] = perm[i];
        p_[i + 256] = perm[i];
    }
}

double PerlinNoise::fade(double t) const {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

double PerlinNoise::lerp(double t, double a, double b) const {
    return a + t * (b - a);
}

double PerlinNoise::grad(int hash, double x, double y) const {
    int h = hash & 3;
    double u = h < 2 ? x : y;
    double v = h < 2 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

double PerlinNoise::noise(double x, double y) const {
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;

    x -= floor(x);
    y -= floor(y);

    double u = fade(x);
    double v = fade(y);

    int A  = p_[X] + Y;
    int AA = p_[A];
    int AB = p_[A + 1];
    int B  = p_[X + 1] + Y;
    int BA = p_[B];
    int BB = p_[B + 1];

    return lerp(v,
        lerp(u, grad(p_[AA], x, y),     grad(p_[BA], x - 1, y)),
        lerp(u, grad(p_[AB], x, y - 1), grad(p_[BB], x - 1, y - 1))
    );
}

double PerlinNoise::octaveNoise(double x, double y, int octaves, double persistence) const {
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    double maxValue = 0.0;

    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0;
    }

    return total / maxValue;
}