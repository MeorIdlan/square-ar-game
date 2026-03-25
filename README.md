# Square AR Game

This repository contains the first implementation milestone for a physical AR floor elimination game.

## Current status

The project currently provides:

- a runnable PyQt6 control window
- projector and debug windows wired to shared render state
- persisted settings for grid, display, and timing values
- core enums, models, and DTO-style contracts
- game-engine and floor-mapping foundation logic
- OpenCV-backed camera capture with fallback when no webcam is available
- ArUco-based one-shot calibration that requires all 4 configured markers visible
- configurable ArUco dictionary selection, supporting 4x4/5x5/6x6/7x7 families with 50/100/250/1000 variants, defaulting to `DICT_6X6_1000`
- MediaPipe Tasks pose tracking using a local pose landmarker model asset
- live projector rendering over the camera frame with projected cell overlays and player markers
- worker scaffolding for camera and pose threads
- floor-space player identity tracking with nearest-neighbor matching
- short missed-detection grace-period handling before players become missing
- automatic round phase progression using configurable timings
- operator controls for forcing the next round, resetting the session, and overriding player outcomes
- deterministic elimination checks for missing, ambiguous, out-of-bounds, and red-cell outcomes

Camera-backed gameplay validation and packaging are the main remaining milestones.

## Platform notes

### Linux runtime note

For MediaPipe Tasks to initialize correctly on Linux, install a GLES runtime first:

```bash
sudo apt install libgles2-mesa
```

### Windows prerequisites

If you run the game from source on Windows, install these first:

1. Python 3.13 x64
2. Microsoft Visual C++ Redistributable 2015-2022 x64
3. A working webcam driver for the camera you plan to use

Then install the Python packages from this repository:

```bash
py -3.13 -m pip install -r requirements.txt
```

If you later run a PyInstaller build on Windows, the Visual C++ Redistributable and camera drivers should still be installed on the target machine.

### PyInstaller note

When packaging later, make sure the bundle includes:

1. the MediaPipe pose model at `assets/models/pose_landmarker_lite.task`
2. the OpenCV and MediaPipe native libraries collected by PyInstaller
3. the Qt platform plugins required by PyQt6

This repository now includes:

1. a committed PyInstaller spec at `square_ar_game.spec`
2. a Windows GitHub Actions build workflow at `.github/workflows/build-windows-exe.yml`
3. a separate build dependency file at `requirements-build.txt`

Release packaging behavior:

1. pushes to `main` and pull requests build the Windows distribution and upload it as a workflow artifact
2. pushing a tag like `v0.1.0` builds the Windows distribution, zips it, creates or updates a GitHub Release for that tag, and attaches the zip as a downloadable release asset

To build locally with PyInstaller:

```bash
py -3.13 -m pip install -r requirements.txt
py -3.13 -m pip install -r requirements-build.txt
py -3.13 -m PyInstaller --noconfirm --clean square_ar_game.spec
```

The Windows build artifact produced by GitHub Actions is an unpacked PyInstaller distribution, which is the right default for validating OpenCV, MediaPipe, and Qt runtime behavior before attempting a single-file build.

## Run

Create and activate a Python environment, install the dependencies, then run:

```bash
python -m src.main
```

## Next implementation targets

1. Validate live camera and projector selection against real hardware.
2. Tune calibration recovery and operator-facing error messaging.
3. Improve the projector and debug views with deeper calibration overlays.
4. Add operator-facing camera health and reconnect feedback.

## What can still be built before a camera is connected?

The most useful remaining milestones that do not require live hardware are:

1. richer projector and debug diagnostics for calibration and tracking state
2. manual calibration diagnostics and recovery messaging
3. operator-facing camera health and reconnect messaging
