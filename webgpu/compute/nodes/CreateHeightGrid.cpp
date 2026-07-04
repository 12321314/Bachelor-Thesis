#include "CreateHeightGrid.h"
#include "util.h"
#include <QImage>
#include <QColor>

#include <QDebug>
#include <QFileInfo>
#include <set>

namespace webgpu_compute::nodes {

CreateHeightGrid::CreateHeightGrid(const CreateHeightGridSettings& settings)
    : Node()
{
    set_settings(settings);
}

CreateHeightGrid::CreateHeightGrid()
    : CreateHeightGrid(CreateHeightGridSettings())
{
}

void CreateHeightGrid::run_impl()
{

}



void CreateHeightGrid::set_settings(const CreateHeightGridSettings& settings)
{

}

void CreateHeightGrid::check_progress_and_emit_signals()
{

}

void CreateHeightGrid::serialize_settings(QJsonObject& out) const
{

}

void CreateHeightGrid::deserialize_settings(const QJsonObject& in)
{

}

} // namespace webgpu_compute::nodes


