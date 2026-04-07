from __future__ import annotations

from unittest.mock import MagicMock

from src.models.game_session_model import GameSessionModel
from src.utils.config import AppConfig, ConfigStore
from src.viewmodels.config_coordinator import ConfigCoordinator


def _make_coordinator(
    tmp_path=None,
) -> tuple[ConfigCoordinator, AppConfig, GameSessionModel, ConfigStore]:
    config = AppConfig()
    session = GameSessionModel()
    session.grid.rows = config.grid.rows
    session.grid.columns = config.grid.columns
    session.grid.reset_states()
    store = MagicMock(spec=ConfigStore)
    coordinator = ConfigCoordinator(config, store, session)
    return coordinator, config, session, store


class TestConfigCoordinator:
    def test_update_grid_rows_propagates(self, qtbot) -> None:
        coordinator, config, session, _ = _make_coordinator()
        with qtbot.waitSignal(coordinator.config_changed, timeout=1000):
            coordinator.update_grid_rows(6)
        assert config.grid.rows == 6
        assert session.grid.rows == 6
        assert len(session.grid.cell_states) == 6 * config.grid.columns

    def test_update_grid_columns_propagates(self, qtbot) -> None:
        coordinator, config, session, _ = _make_coordinator()
        with qtbot.waitSignal(coordinator.config_changed, timeout=1000):
            coordinator.update_grid_columns(8)
        assert config.grid.columns == 8
        assert session.grid.columns == 8

    def test_update_timing_setting(self, qtbot) -> None:
        coordinator, config, session, _ = _make_coordinator()
        with qtbot.waitSignal(coordinator.config_changed, timeout=1000):
            coordinator.update_timing_setting("flashing_duration_seconds", 5.0)
        assert config.timings.flashing_duration_seconds == 5.0
        assert session.round_state.timings.flashing_duration_seconds == 5.0

    def test_update_timing_invalid_field_ignored(self) -> None:
        coordinator, config, _, _ = _make_coordinator()
        coordinator.update_timing_setting("nonexistent_field", 99.0)
        # Should not raise

    def test_save_config_calls_store(self) -> None:
        coordinator, config, _, store = _make_coordinator()
        coordinator.save_config()
        store.save.assert_called_once_with(config)

    def test_autosave_config_calls_store(self) -> None:
        coordinator, config, _, store = _make_coordinator()
        coordinator.autosave_config()
        store.save.assert_called_once_with(config)
