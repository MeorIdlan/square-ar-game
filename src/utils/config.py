from __future__ import annotations

from dataclasses import asdict, dataclass, field
import json
from pathlib import Path

from src.models.round_model import RoundTimingSettings


@dataclass(slots=True)
class GridSettings:
    rows: int = 4
    columns: int = 4
    playable_width: float = 4.0
    playable_height: float = 4.0
    playable_inset: float = 0.2


@dataclass(slots=True)
class CameraSettings:
    camera_index: int = 0
    frame_width: int = 1280
    frame_height: int = 720
    target_fps: int = 30


@dataclass(slots=True)
class DisplaySettings:
    projector_screen_index: int = 0
    show_debug_window: bool = True
    show_player_markers: bool = True


@dataclass(slots=True)
class PoseSettings:
    model_asset_path: str = "assets/models/pose_landmarker_full.task"
    num_poses: int = 4
    min_pose_detection_confidence: float = 0.35
    min_pose_presence_confidence: float = 0.35
    min_tracking_confidence: float = 0.35
    min_landmark_visibility: float = 0.2


@dataclass(slots=True)
class AppConfig:
    camera: CameraSettings = field(default_factory=CameraSettings)
    display: DisplaySettings = field(default_factory=DisplaySettings)
    pose: PoseSettings = field(default_factory=PoseSettings)
    grid: GridSettings = field(default_factory=GridSettings)
    timings: RoundTimingSettings = field(default_factory=RoundTimingSettings)
    aruco_dictionary: str = "DICT_6X6_1000"
    marker_ids: list[int] = field(default_factory=lambda: [0, 1, 2, 3])


class ConfigStore:
    def __init__(self, config_path: Path | None = None) -> None:
        self._config_path = config_path or Path.home() / ".square-ar-game" / "settings.json"
        self._config_path.parent.mkdir(parents=True, exist_ok=True)

    @property
    def config_path(self) -> Path:
        return self._config_path

    def load(self) -> AppConfig:
        if not self._config_path.exists():
            return AppConfig()

        raw_data = json.loads(self._config_path.read_text(encoding="utf-8"))
        pose_settings = PoseSettings(**raw_data.get("pose", {}))
        if pose_settings.model_asset_path == "assets/models/pose_landmarker_lite.task":
            pose_settings.model_asset_path = "assets/models/pose_landmarker_full.task"
        if pose_settings.min_pose_detection_confidence == 0.5:
            pose_settings.min_pose_detection_confidence = 0.35
        if pose_settings.min_pose_presence_confidence == 0.5:
            pose_settings.min_pose_presence_confidence = 0.35
        if pose_settings.min_tracking_confidence == 0.5:
            pose_settings.min_tracking_confidence = 0.35
        if pose_settings.min_landmark_visibility == 0.4:
            pose_settings.min_landmark_visibility = 0.2

        return AppConfig(
            camera=CameraSettings(**raw_data.get("camera", {})),
            display=DisplaySettings(**raw_data.get("display", {})),
            pose=pose_settings,
            grid=GridSettings(**raw_data.get("grid", {})),
            timings=RoundTimingSettings(**raw_data.get("timings", {})),
            aruco_dictionary=raw_data.get("aruco_dictionary", "DICT_6X6_1000"),
            marker_ids=raw_data.get("marker_ids", [0, 1, 2, 3]),
        )

    def save(self, config: AppConfig) -> None:
        payload = asdict(config)
        self._config_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")
