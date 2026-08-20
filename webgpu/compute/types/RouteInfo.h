#pragma once
#include "glm/vec4.hpp"

#include <vector>

struct RouteInfo {
    std::vector<glm::vec4> path;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec4> points;
};