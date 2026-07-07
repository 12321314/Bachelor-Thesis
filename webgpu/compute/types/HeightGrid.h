#pragma once

#include <vector>
#include <cstdint>

struct HeightGrid {

    uint32_t width;
    uint32_t height;

    double origin_x;
    double origin_y;

    float cell_size_meters;

    std::vector<float> heights;
};