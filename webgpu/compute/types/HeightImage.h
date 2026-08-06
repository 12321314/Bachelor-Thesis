#pragma once

#include <vector>
#include <cstdint>

struct HeightImage {

    int width = 0;
    int height = 0;

    std::vector<uint8_t> pixels;
};