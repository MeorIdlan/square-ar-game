from __future__ import annotations

import sys
from pathlib import Path
from unittest.mock import patch

from src.utils.app_paths import application_root, log_directory


class TestApplicationRoot:
    def test_dev_mode_returns_project_root(self) -> None:
        root = application_root()
        assert (root / "assets").is_dir()
        assert (root / "python" / "src").is_dir()

    def test_frozen_with_meipass(self, tmp_path: Path) -> None:
        fake_meipass = str(tmp_path / "bundle")
        (tmp_path / "bundle").mkdir()
        with (
            patch.object(sys, "frozen", True, create=True),
            patch.object(sys, "_MEIPASS", fake_meipass, create=True),
        ):
            result = application_root()
            assert result == (tmp_path / "bundle").resolve()

    def test_frozen_without_meipass(self, tmp_path: Path) -> None:
        with (
            patch.object(sys, "frozen", True, create=True),
            patch.object(sys, "_MEIPASS", None, create=True),
            patch.object(sys, "executable", str(tmp_path / "app.exe")),
        ):
            result = application_root()
            assert result == tmp_path.resolve()


class TestLogDirectory:
    def test_returns_path(self) -> None:
        result = log_directory()
        assert isinstance(result, Path)
        assert result.name == "logs"
