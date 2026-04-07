from __future__ import annotations

import logging

from PyQt6.QtCore import QObject, pyqtSignal

from src.models.game_session_model import GameSessionModel
from src.utils.config import AppConfig, ConfigStore


class ConfigCoordinator(QObject):
    config_changed = pyqtSignal()

    def __init__(
        self,
        config: AppConfig,
        config_store: ConfigStore,
        session_model: GameSessionModel,
    ) -> None:
        super().__init__()
        self._config = config
        self._config_store = config_store
        self._session_model = session_model
        self._logger = logging.getLogger(__name__)

    def update_grid_rows(self, value: int) -> None:
        self._config.grid.rows = value
        self._session_model.grid.rows = value
        self._session_model.grid.reset_states()
        self.config_changed.emit()

    def update_grid_columns(self, value: int) -> None:
        self._config.grid.columns = value
        self._session_model.grid.columns = value
        self._session_model.grid.reset_states()
        self.config_changed.emit()

    def update_timing_setting(self, field_name: str, value: float) -> None:
        if not hasattr(self._config.timings, field_name):
            return
        setattr(self._config.timings, field_name, value)
        setattr(self._session_model.round_state.timings, field_name, value)
        self.config_changed.emit()

    def update_aruco_dictionary(self, dictionary_name: str) -> None:
        from src.services.calibration_service import CalibrationService

        if dictionary_name not in CalibrationService.supported_dictionary_names():
            return
        self._config.aruco_dictionary = dictionary_name
        self._logger.info("ArUco dictionary updated to %s", dictionary_name)
        self.config_changed.emit()

    def save_config(self) -> None:
        self._config_store.save(self._config)
        self._logger.info("Saved settings to %s", self._config_store.config_path)

    def autosave_config(self) -> None:
        self._config_store.save(self._config)
        self._logger.info("Autosaved settings to %s", self._config_store.config_path)

    @property
    def config_path(self) -> str:
        return str(self._config_store.config_path)
