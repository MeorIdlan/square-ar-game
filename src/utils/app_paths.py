from __future__ import annotations

from pathlib import Path
import sys


def application_root() -> Path:
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent
    return Path(__file__).resolve().parents[2]


def internal_runtime_root() -> Path:
    root = application_root()
    if getattr(sys, "frozen", False):
        return root / "_internal"
    return root


def log_directory() -> Path:
    base = internal_runtime_root() / "logs"
    base.mkdir(parents=True, exist_ok=True)
    return base
