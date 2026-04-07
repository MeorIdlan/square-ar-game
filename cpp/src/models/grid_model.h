#pragma once
// GridModel — mirrors src/models/grid_model.py

#include "models/enums.h"
#include "utils/math_helpers.h"

#include <unordered_map>

namespace sag {

struct CellGeometry {
    CellIndex index;
    Point top_left;
    Point bottom_right;

    float width()  const { return bottom_right.x - top_left.x; }
    float height() const { return bottom_right.y - top_left.y; }
};

using CellStateMap = std::unordered_map<CellIndex, CellState, CellIndexHash>;

class GridModel {
public:
    int rows           = 4;
    int columns        = 4;
    float playable_width  = 4.0f;
    float playable_height = 4.0f;

    CellStateMap cell_states;

    GridModel();
    GridModel(int rows, int columns, float pw = 4.0f, float ph = 4.0f);

    void reset_states(CellState state = CellState::Neutral);

    float cell_width()  const { return playable_width / static_cast<float>(columns); }
    float cell_height() const { return playable_height / static_cast<float>(rows); }

    CellGeometry cell_geometry(int row, int column) const;
};

} // namespace sag
