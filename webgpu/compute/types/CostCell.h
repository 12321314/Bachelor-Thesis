#pragma once

struct CostCell {
    float elevation;
    float slope;

    float cost;

    bool selected_one;
    bool selected_two;
    bool selected_three; //TODO update the overleaf listing with this new stuff
    bool controlPoint = false;

    bool forest;
    bool path;
    bool stream;
    bool river;
    bool waterbody;

    float gCost = 9999999.0f; // real costs from start until this point
    float hCost = 0.0f;       // evaluated costs
    float fCost = 9999999.0f; // gCost + hCost
    float simulated_cost = 0.0f;
    bool visited = false;

    int x = -1;
    int y = -1;
    int parentX = -1; // points back at the previous cell during path finding
    int parentY = -1;
};