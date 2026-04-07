#include "models/grid_model.h"

namespace sag {

GridModel::GridModel() {
    reset_states();
}

GridModel::GridModel(int rows, int columns, float pw, float ph)
    : rows(rows), columns(columns), playable_width(pw), playable_height(ph)
{
    reset_states();
}

void GridModel::reset_states(CellState state) {
    cell_states.clear();
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columns; ++c) {
            cell_states[{r, c}] = state;
        }
    }
}

CellGeometry GridModel::cell_geometry(int row, int column) const {
    float cw = cell_width();
    float ch = cell_height();
    Point tl{static_cast<float>(column) * cw, static_cast<float>(row) * ch};
    Point br{static_cast<float>(column + 1) * cw, static_cast<float>(row + 1) * ch};
    return CellGeometry{{row, column}, tl, br};
}

} // namespace sag
