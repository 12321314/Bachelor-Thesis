#pragma once

#include <utility>
#include <vector>

struct Way {
    std::vector<std::pair<double, double>> points;

    bool forest;
    bool path;
    bool stream;
    bool river;
    bool waterbody;
};