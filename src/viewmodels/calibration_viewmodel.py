from __future__ import annotations

from PyQt6.QtCore import QObject, pyqtSignal

from src.models.contracts import FramePacket

from src.models.calibration_model import CalibrationModel
from src.services.protocols import CalibrationServiceProtocol


class CalibrationViewModel(QObject):
    calibration_updated = pyqtSignal(object)

    def __init__(
        self,
        calibration_service: CalibrationServiceProtocol,
        calibration_model: CalibrationModel,
    ) -> None:
        super().__init__()
        self._calibration_service = calibration_service
        self._calibration_model = calibration_model

    @property
    def calibration_model(self) -> CalibrationModel:
        return self._calibration_model

    def calibrate(self, frame_packet: FramePacket | None) -> None:
        frame = None if frame_packet is None else frame_packet.frame
        self._calibration_model = self._calibration_service.calibrate_from_frame(
            self._calibration_model,
            frame,
            is_live_source=False if frame_packet is None else frame_packet.is_live,
            source_error=None if frame_packet is None else frame_packet.error_message,
        )
        self.calibration_updated.emit(self._calibration_model)

    def reset(self) -> None:
        self._calibration_model = self._calibration_service.reset(
            self._calibration_model
        )
        self.calibration_updated.emit(self._calibration_model)
