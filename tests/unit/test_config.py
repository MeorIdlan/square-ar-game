from __future__ import annotations

import json
from pathlib import Path

import pytest

from src.utils.config import (
    AppConfig,
    CameraProfile,
    CameraSettings,
    ConfigStore,
)


class TestCameraProfile:
    def test_frozen(self) -> None:
        profile = CameraProfile(1280, 720, 30)
        with pytest.raises(AttributeError):
            profile.width = 1920  # type: ignore[misc]

    def test_label_hd(self) -> None:
        assert CameraProfile(1280, 720, 30).label == "720p 30 FPS (1280x720)"

    def test_label_4k(self) -> None:
        assert CameraProfile(3840, 2160, 30).label == "4K 30 FPS"


class TestAppConfig:
    def test_default_construction(self) -> None:
        config = AppConfig()
        assert config.camera.camera_index == 0
        assert config.aruco_dictionary == "DICT_6X6_1000"
        assert config.marker_ids == [0, 1, 2, 3]

    def test_camera_settings_profile(self) -> None:
        settings = CameraSettings(frame_width=1920, frame_height=1080, target_fps=60)
        profile = settings.profile
        assert profile.width == 1920
        assert profile.height == 1080
        assert profile.fps == 60


class TestConfigStoreLoad:
    def test_missing_file_returns_defaults(self, tmp_path: Path) -> None:
        store = ConfigStore(config_path=tmp_path / "nonexistent.json")
        config = store.load()
        assert config.camera.camera_index == 0
        assert config.aruco_dictionary == "DICT_6X6_1000"

    def test_valid_json(self, tmp_path: Path) -> None:
        config_path = tmp_path / "settings.json"
        config_path.write_text(
            json.dumps(
                {
                    "camera": {
                        "camera_index": 2,
                        "frame_width": 1920,
                        "frame_height": 1080,
                        "target_fps": 60,
                    },
                    "aruco_dictionary": "DICT_4X4_50",
                }
            ),
            encoding="utf-8",
        )
        store = ConfigStore(config_path=config_path)
        config = store.load()
        assert config.camera.camera_index == 2
        assert config.aruco_dictionary == "DICT_4X4_50"

    def test_corrupt_json_returns_error(self, tmp_path: Path) -> None:
        config_path = tmp_path / "settings.json"
        config_path.write_text("not json!", encoding="utf-8")
        store = ConfigStore(config_path=config_path)
        with pytest.raises(json.JSONDecodeError):
            store.load()


class TestConfigStoreMigrate:
    def test_confidence_migration_from_0_5(self) -> None:
        raw = {
            "pose": {
                "min_pose_detection_confidence": 0.5,
                "min_pose_presence_confidence": 0.5,
                "min_tracking_confidence": 0.5,
                "min_landmark_visibility": 0.4,
            }
        }
        result = ConfigStore._migrate(raw)
        assert result["pose"]["min_pose_detection_confidence"] == 0.35
        assert result["pose"]["min_pose_presence_confidence"] == 0.35
        assert result["pose"]["min_tracking_confidence"] == 0.35
        assert result["pose"]["min_landmark_visibility"] == 0.2

    def test_no_migration_when_already_correct(self) -> None:
        raw = {
            "pose": {
                "min_pose_detection_confidence": 0.35,
                "min_pose_presence_confidence": 0.35,
                "min_tracking_confidence": 0.35,
                "min_landmark_visibility": 0.2,
            }
        }
        result = ConfigStore._migrate(raw)
        assert result["pose"]["min_pose_detection_confidence"] == 0.35


class TestConfigStoreSave:
    def test_round_trip(self, tmp_path: Path) -> None:
        config_path = tmp_path / "settings.json"
        store = ConfigStore(config_path=config_path)
        original = AppConfig()
        original.camera.camera_index = 5
        store.save(original)
        loaded = store.load()
        assert loaded.camera.camera_index == 5
