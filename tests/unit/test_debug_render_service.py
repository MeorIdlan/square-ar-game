from __future__ import annotations

from src.models.contracts import RenderState
from src.models.enums import CellState
from src.services.debug_render_service import DebugRenderService
from src.utils.constants import DEBUG_CANVAS_SIZE


class TestDebugRenderService:
    def test_render_returns_correct_size(self) -> None:
        service = DebugRenderService()
        render_state = RenderState()
        image = service.render(DEBUG_CANVAS_SIZE[0], DEBUG_CANVAS_SIZE[1], render_state)
        assert image.width() == 640
        assert image.height() == 640

    def test_all_cell_states_render_without_exception(self) -> None:
        service = DebugRenderService()
        grid_cells = {
            (0, 0): CellState.GREEN,
            (0, 1): CellState.RED,
            (1, 0): CellState.FLASHING,
            (1, 1): CellState.NEUTRAL,
            (2, 0): CellState.INACTIVE,
            (2, 1): CellState.LOCKED,
        }
        render_state = RenderState(grid_cells=grid_cells)
        image = service.render(640, 640, render_state)
        assert image.width() == 640
        assert image.height() == 640
