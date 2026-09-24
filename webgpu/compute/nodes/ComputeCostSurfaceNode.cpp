#include "ComputeCostSurfaceNode.h"
#include "nucleus/srs.h"
#include "../types/CostCell.h"
#include "util.h"



#include <QDebug>
#include <QFileInfo>
#include <set>

namespace webgpu_compute::nodes {

ComputeCostSurfaceNode::ComputeCostSurfaceNode(const ComputeCostSurfaceNodeSettings& settings)
: Node(
      {
          InputSocket(*this, "height grid", data_type<const HeightGrid*>()),
          InputSocket(*this, "OSM ways", data_type<const std::vector<Way>*>())
      },
      {
          OutputSocket(*this, "cost grid", data_type<const CostGrid*>(), [this]{return &m_cost_grid;}),
      })
{
    set_settings(settings);
}

ComputeCostSurfaceNode::ComputeCostSurfaceNode()
    : ComputeCostSurfaceNode(ComputeCostSurfaceNodeSettings())
{
}

void ComputeCostSurfaceNode::run_impl()
{
    const auto height_grid = std::get<data_type<const HeightGrid*>()>(input_socket("height grid").get_connected_data());
    const auto ways = std::get<data_type<const std::vector<Way>*>()>(input_socket("OSM ways").get_connected_data());

    // -----------------------------
    // Creating Cost Grid
    // -----------------------------

    width = height_grid->width;
    height = height_grid->height;
    m_cost_grid.costs.clear();
    m_cost_grid.costs.reserve(width * height); //cost grid
    m_cost_grid.width = width;
    m_cost_grid.height = height;
    m_cost_grid.origin_x = height_grid->origin_x;
    m_cost_grid.origin_y = height_grid->origin_y;
    m_cost_grid.cell_size_meters = height_grid->cell_size_meters;

    for (int y = 0; y < height; ++y) {

        for (int x = 0; x < width; ++x) {

            CostCell cell;
            cell.elevation = height_grid->heights[y * width + x];
            cell.forest = false;
            cell.path = false;
            cell.stream = false;
            cell.river = false;
            cell.waterbody = false;
            cell.slope = 0;
            cell.cost = 0;
            cell.selected_one = false;
            cell.selected_two = false;
            cell.selected_three = false;
            cell.x = x;
            cell.y = y;

            m_cost_grid.costs.push_back(cell);
        }
    }

    // -----------------------------
    // Applying OSM Data
    // -----------------------------

    for (auto& way : *ways) {

        int oldX = -1;
        int oldY = -1;

        int firstX = -1;
        int firstY = -1;

        for (int i = 0; i < way.points.size(); i++) {

            double lat1 = way.points[i].first;
            double lon1 = way.points[i].second;
            glm::dvec2 lat_lon = glm::dvec2(lat1, lon1);

            lat_lon = nucleus::srs::lat_long_to_world(lat_lon);

            int x1 = (int)((lat_lon.x - height_grid->origin_x) / height_grid->cell_size_meters);
            int y1 = (int)((height_grid->origin_y - lat_lon.y) / height_grid->cell_size_meters);


            if (y1 >= 0 && y1 < height && x1 >= 0 && x1 < width) {
                m_cost_grid.costs[y1 * width + x1].forest = way.forest;
                m_cost_grid.costs[y1 * width + x1].path = way.path;
                m_cost_grid.costs[y1 * width + x1].stream = way.stream;
                m_cost_grid.costs[y1 * width + x1].river = way.river;
                m_cost_grid.costs[y1 * width + x1].waterbody = way.waterbody;

                // waypoints get interpolated with their previous point
                if (oldX != -1 && oldY != -1) {
                    interpolateLine(x1, y1, oldX, oldY, way);
                }

                oldX = x1;
                oldY = y1;
            }
            else {
                // important so the next waypoint doesnt get interpolated with faulty coords
                oldX = -1;
                oldY = -1;
            }
        }
    }

    // -----------------------------
    // Calculating Costs
    // -----------------------------

    calculateSlope(height_grid->cell_size_meters);
    calculateCost();

    complete_run();
}

void ComputeCostSurfaceNode::interpolateLine(int x1, int y1, int oldX, int oldY, Way way) {

    int oldDataX = oldX;
    int oldDataY = oldY;

    int dx = abs(x1 - oldX);
    int dy = -abs(y1 - oldY);

    int sx = (oldX < x1) ? 1 : -1;
    int sy = (oldY < y1) ? 1 : -1;

    int err = dx + dy;

    while (oldX != x1 || oldY != y1) {
        if (oldX >= 0 && oldX < width && oldY >= 0 && oldY < height) {
            m_cost_grid.costs[oldY * width + oldX].path = way.path;
            m_cost_grid.costs[oldY * width + oldX].stream = way.stream;
            m_cost_grid.costs[oldY * width + oldX].river = way.river;
            m_cost_grid.costs[oldY * width + oldX].waterbody = way.waterbody;
            m_cost_grid.costs[oldY * width + oldX].forest = way.forest;
        }

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            oldX += sx;
        }

        if (e2 <= dx) {
            err += dx;
            oldY += sy;
        }
    }
}

void ComputeCostSurfaceNode::calculateSlope(float cellSize) {
    float biggestSlope = 0.0f;
    float smallestSlope = 5000.0f;
    float averageSlope = 0.0f;

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {

            float left  = m_cost_grid.costs[y * width + (x - 1)].elevation;
            float right = m_cost_grid.costs[y * width + (x + 1)].elevation;

            float up    = m_cost_grid.costs[(y - 1) * width + x].elevation;
            float down  = m_cost_grid.costs[(y + 1) * width + x].elevation;

            float dzdx = (right - left) / (2.0f * cellSize);
            float dzdy = (down - up) / (2.0f * cellSize);

            float gradient = sqrt(dzdx * dzdx + dzdy * dzdy);

            float slopeRad = atan(gradient);

            float slopeDeg = slopeRad * 180.0f / M_PI;

            m_cost_grid.costs[y * width + x].slope = slopeDeg;

            // gathering important data
            if (slopeDeg > biggestSlope) {
                biggestSlope = slopeDeg;
            }
            if (slopeDeg < smallestSlope) {
                smallestSlope = slopeDeg;
            }
            averageSlope += slopeDeg;
        }
    }

    averageSlope = averageSlope / ((width - 2) * (height - 2));

    // return some important infos
    qInfo() << "Smallest Slope: " << smallestSlope;
    qInfo() << "Biggest Slope: " << biggestSlope;
    qInfo() << "Average Slope: " << averageSlope;

}

void ComputeCostSurfaceNode::calculateCost() {
    float cost = 0; // can go from 0 to 99

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (y == 0 || x == 0 || y == height - 1 || x == width - 1) {
                cost = 99;
            }
            else {
                CostCell cell = m_cost_grid.costs[y * width + x];
                if (cell.forest) {
                    cost = 20 * (m_settings.m_forest_influence / 10) + cell.slope * 1.5;
                }
                else {
                    cost = cell.slope * 1.5;
                }

                if (cell.river && !cell.path) {
                    cost += 20 * (m_settings.m_river_influence / 10);
                }
                else if (cell.stream && !cell.path) {
                    cost += 10 * (m_settings.m_stream_influence / 10);
                }

                if (cell.path) {
                    cost -= 30 * (m_settings.m_path_influence / 10);
                }

                if (cell.waterbody && !cell.stream && !cell.river) {
                    cost = 99;
                }

                if (cost > 99) {
                    cost = 99;
                }
                if (cost < 0) {
                    cost = 0;
                }
            }

            m_cost_grid.costs[y * width + x].cost = cost;
            m_cost_grid.costs[y * width + x].simulated_cost = cost;
        }
    }

}

void ComputeCostSurfaceNode::set_settings(const ComputeCostSurfaceNodeSettings& settings)
{
    m_settings = settings;
}

void ComputeCostSurfaceNode::check_progress_and_emit_signals()
{
    complete_run();
}

void ComputeCostSurfaceNode::serialize_settings(QJsonObject& out) const
{
}

void ComputeCostSurfaceNode::deserialize_settings(const QJsonObject& in)
{
}

} // namespace webgpu_compute::nodes


