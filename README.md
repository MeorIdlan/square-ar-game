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
- projector and debug HUD diagnostics for camera, display, phase, timer, and calibration status
- worker scaffolding for camera and pose threads
- floor-space player identity tracking with nearest-neighbor matching
- short missed-detection grace-period handling before players become missing
- automatic round phase progression using configurable timings
- operator controls for forcing the next round, resetting the session, and overriding player outcomes
- operator controls for rescanning camera indices, reconnecting the selected camera, and refreshing display enumeration
- explicit live-vs-fallback camera diagnostics so calibration is blocked when the app is using a synthetic fallback frame
- deterministic elimination checks for missing, ambiguous, out-of-bounds, and red-cell outcomes
- persisted settings at `~/.square-ar-game/settings.json`
- PyInstaller packaging for Windows via GitHub Actions, including tag-triggered GitHub Release assets

The main remaining work is real-hardware validation and tuning, not core scaffolding.

## Platform notes

### Linux runtime note

For MediaPipe Tasks to initialize correctly on Linux, install a GLES runtime first:

```bash
sudo apt install libgles2-mesa
```

If you are running under WSL2, the app can render Qt windows and enumerate displays, but camera access still depends on the webcam being exposed into the Linux environment. If no `/dev/video*` device is present, the app will switch to its fallback frame and report that in the control window, projector HUD, and debug window.

### Windows prerequisites

If you run the game from source on Windows, install these first:

1. Python 3.13 x64
2. Microsoft Visual C++ Redistributable 2015-2022 x64
3. A working webcam driver for the camera you plan to use
4. A graphics driver stack capable of running PyQt6, OpenCV, and MediaPipe Tasks

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

On Windows, the equivalent command is:

```bash
py -3.13 -m src.main
```

## Operator workflow

1. Start the app and confirm the control window shows a live camera status rather than fallback mode.
2. Use `Rescan Cameras` if camera indices have changed or the device was connected after startup.
3. Use `Reconnect Camera` if the selected camera stopped responding.
4. Confirm the target projector display in `Projector Display`, then use `Refresh Displays` if monitors were added or removed.
5. Present all four configured ArUco markers and run calibration.
6. Start the round only after calibration reports a valid lock.

## Calibration behavior

- Calibration requires all four configured marker IDs to be visible in one frame.
- The default marker IDs are `0, 1, 2, 3`.
- The default ArUco dictionary is `DICT_6X6_1000`.
- Calibration is rejected when the app is using a fallback frame instead of a live camera image.
- Calibration diagnostics distinguish between missing markers, invalid marker geometry, homography failure, and unavailable camera input.

## Next implementation targets

1. Validate live camera capture end-to-end on native Windows hardware.
2. Tune marker detection thresholds and pose mapping against the real play area.
3. Add richer calibration overlays for marker placement quality and homography confidence.
4. Expose marker ID editing in the operator UI if field setup needs it.

## What can still be built before a camera is connected?

The most useful remaining milestones that do not require live hardware are now:

1. marker-ID editing and additional persisted setup controls
2. richer calibration overlays and operator guidance text
3. packaging polish and release automation refinements
