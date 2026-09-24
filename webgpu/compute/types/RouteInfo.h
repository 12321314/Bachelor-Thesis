#pragma once
#include "glm/vec4.hpp"

#include <vector>

struct RouteInfo {
    std::vector<glm::vec4> path_one;
    std::vector<glm::vec4> path_two;
    std::vector<glm::vec4> path_three;
    std::vector<glm::vec4> colors_one;
    std::vector<glm::vec4> colors_two;
    std::vector<glm::vec4> colors_three;
    std::vector<glm::vec4> points;
};