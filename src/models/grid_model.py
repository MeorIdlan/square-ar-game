from __future__ import annotations

from dataclasses import dataclass, field

from src.models.enums import CellState


Point = tuple[float, float]
CellIndex = tuple[int, int]


@dataclass(frozen=True, slots=True)
class CellGeometry:
    index: CellIndex
    top_left: Point
    bottom_right: Point

    @property
    def width(self) -> float:
        return self.bottom_right[0] - self.top_left[0]

    @property
    def height(self) -> float:
        return self.bottom_right[1] - self.top_left[1]


@dataclass(slots=True)
class GridModel:
    rows: int = 4
    columns: int = 4
    playable_width: float = 4.0
    playable_height: float = 4.0
    cell_states: dict[CellIndex, CellState] = field(default_factory=dict)

    def __post_init__(self) -> None:
        if not self.cell_states:
            self.reset_states()

    def reset_states(self, cell_state: CellState = CellState.NEUTRAL) -> None:
        self.cell_states = {
            (row, column): cell_state
            for row in range(self.rows)
            for column in range(self.columns)
        }

    @property
    def cell_width(self) -> float:
        return self.playable_width / self.columns

    @property
    def cell_height(self) -> float:
        return self.playable_height / self.rows

    def cell_geometry(self, row: int, column: int) -> CellGeometry:
        top_left = (column * self.cell_width, row * self.cell_height)
        bottom_right = ((column + 1) * self.cell_width, (row + 1) * self.cell_height)
        return CellGeometry(index=(row, column), top_left=top_left, bottom_right=bottom_right)
