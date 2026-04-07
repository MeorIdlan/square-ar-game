#include <gtest/gtest.h>
#include "models/grid_model.h"

using namespace sag;

TEST(GridModelTest, DefaultConstruction) {
    GridModel grid;
    EXPECT_EQ(grid.rows, 4);
    EXPECT_EQ(grid.columns, 4);
    EXPECT_FLOAT_EQ(grid.playable_width, 4.0f);
    EXPECT_FLOAT_EQ(grid.playable_height, 4.0f);
    EXPECT_EQ(grid.cell_states.size(), 16u);
}

TEST(GridModelTest, CustomSize) {
    GridModel grid(3, 5, 6.0f, 3.0f);
    EXPECT_EQ(grid.rows, 3);
    EXPECT_EQ(grid.columns, 5);
    EXPECT_FLOAT_EQ(grid.playable_width, 6.0f);
    EXPECT_FLOAT_EQ(grid.playable_height, 3.0f);
    EXPECT_EQ(grid.cell_states.size(), 15u);
}

TEST(GridModelTest, CellWidthAndHeight) {
    GridModel grid(4, 4);
    EXPECT_FLOAT_EQ(grid.cell_width(), 1.0f);
    EXPECT_FLOAT_EQ(grid.cell_height(), 1.0f);

    GridModel grid2(2, 8, 8.0f, 4.0f);
    EXPECT_FLOAT_EQ(grid2.cell_width(), 1.0f);
    EXPECT_FLOAT_EQ(grid2.cell_height(), 2.0f);
}

TEST(GridModelTest, CellGeometry) {
    GridModel grid(4, 4);
    auto geo = grid.cell_geometry(0, 0);
    EXPECT_FLOAT_EQ(geo.top_left.x, 0.0f);
    EXPECT_FLOAT_EQ(geo.top_left.y, 0.0f);
    EXPECT_FLOAT_EQ(geo.bottom_right.x, 1.0f);
    EXPECT_FLOAT_EQ(geo.bottom_right.y, 1.0f);
    EXPECT_FLOAT_EQ(geo.width(), 1.0f);
    EXPECT_FLOAT_EQ(geo.height(), 1.0f);

    auto geo2 = grid.cell_geometry(2, 3);
    EXPECT_FLOAT_EQ(geo2.top_left.x, 3.0f);
    EXPECT_FLOAT_EQ(geo2.top_left.y, 2.0f);
    EXPECT_FLOAT_EQ(geo2.bottom_right.x, 4.0f);
    EXPECT_FLOAT_EQ(geo2.bottom_right.y, 3.0f);
}

TEST(GridModelTest, ResetStates) {
    GridModel grid(2, 2);
    grid.cell_states[{0, 0}] = CellState::Red;
    grid.cell_states[{1, 1}] = CellState::Green;

    grid.reset_states(CellState::Neutral);
    for (auto& [idx, state] : grid.cell_states) {
        EXPECT_EQ(state, CellState::Neutral);
    }
}

TEST(GridModelTest, ResetStatesCustom) {
    GridModel grid(2, 2);
    grid.reset_states(CellState::Green);
    for (auto& [idx, state] : grid.cell_states) {
        EXPECT_EQ(state, CellState::Green);
    }
}

TEST(GridModelTest, AllCellsInitializedNeutral) {
    GridModel grid(3, 3);
    EXPECT_EQ(grid.cell_states.size(), 9u);
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            auto it = grid.cell_states.find({r, c});
            ASSERT_NE(it, grid.cell_states.end());
            EXPECT_EQ(it->second, CellState::Neutral);
        }
    }
}
