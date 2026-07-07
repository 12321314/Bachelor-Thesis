#pragma once

#include "CostCell.h"
#include <cstdint>
#include <vector>

struct CostGrid
{
    uint32_t width;
    uint32_t height;
    float cell_size_meters;

    std::vector<CostCell> costs;
};