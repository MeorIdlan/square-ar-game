# -*- mode: python ; coding: utf-8 -*-

import os
from pathlib import Path

from PyInstaller.utils.hooks import collect_data_files, collect_dynamic_libs, collect_submodules


project_root = Path(SPEC).resolve().parent
exe_name = "SquareARGame" if os.name == "nt" else "SquareARGame-bin"

datas = []
datas += collect_data_files("mediapipe")
datas.append((str(project_root / "assets" / "models" / "pose_landmarker_lite.task"), "assets/models"))

binaries = []
binaries += collect_dynamic_libs("cv2")
binaries += collect_dynamic_libs("mediapipe")

hiddenimports = []
hiddenimports += collect_submodules("mediapipe")


a = Analysis(
    [str(project_root / "src" / "main.py")],
    pathex=[str(project_root)],
    binaries=binaries,
    datas=datas,
    hiddenimports=hiddenimports,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    [],
    name=exe_name,
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=False,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)

coll = COLLECT(
    exe,
    a.binaries,
    a.datas,
    strip=False,
    upx=False,
    upx_exclude=[],
    name="SquareARGame",
)
