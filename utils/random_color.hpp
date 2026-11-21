#pragma once

#include <utils/vec3.hpp>
#include <vector>
#include <string>
#include <array>
#include <functional>
#include <random> 
#include <unordered_map>
#include <cstdint>
#include <cmath>
#include <math.h>

struct RandomColor {
    static constexpr float minhue = 0.0f;
    static constexpr float maxhue = 1.0f;
    static constexpr float saturation = 0.8;

    static Vec3 A(std::mt19937 rng) {
        std::uniform_real_distribution<float> dist(minhue, maxhue);
        float h = dist(rng);
        float s = saturation;
        float v = 1.0f;

        // HSV -> RGB
        float c = v * s;
        float hh = h * 6.0f;
        float x = c * (1.0f - std::fabs(std::fmod(hh, 2.0f) - 1.0f));
        float m = v - c;

        float r = 0.0f, g = 0.0f, b = 0.0f;
        if      (hh < 1.0f) { r = c; g = x; b = 0; }
        else if (hh < 2.0f) { r = x; g = c; b = 0; }
        else if (hh < 3.0f) { r = 0; g = c; b = x; }
        else if (hh < 4.0f) { r = 0; g = x; b = c; }
        else if (hh < 5.0f) { r = x; g = 0; b = c; }
        else                { r = c; g = 0; b = x; }

        return Vec3(r+m, g+m, b+m);
    }
};
