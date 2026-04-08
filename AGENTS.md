# square-ar-game — Agent Guidelines

Physical AR floor elimination game with two implementations:

- **Python** (`python/`): Python 3.13, PyQt6, OpenCV (ArUco), MediaPipe pose tracking
- **C++** (`cpp/`): C++20, Direct3D 11, Media Foundation, OpenCV, ONNX Runtime, Dear ImGui

Shared assets live in `assets/` at the repository root.

---

## AI-First TDD Workflow

**All new features and bug fixes start with a failing test. No production code without a test.**

1. **Red** — Write a test that fails for the right reason.
2. **Green** — Write the minimum production code to make it pass.
3. **Refactor** — Clean up; tests must still pass.
4. Run the full suite after every change.

When asked to implement anything, write the test file first, show it failing, then
implement the code. If a behaviour is ambiguous, ask for a failing test that captures
the expected behaviour before writing production code.

---

## Build & Test — Python

All commands run from the `python/` directory.

```bash
cd python/

# Install (dev mode + dev tools)
pip install -e ".[dev]"

# Run all tests with coverage
python -m pytest tests/ -v --tb=short

# Run a single test file
python -m pytest tests/unit/test_round_evaluator.py -v

# Lint (must be clean before any commit)
ruff check src/ tests/ --select E,F,I,UP,B

# Type-check
mypy src/
```

Tests live in `python/tests/unit/`. Shared fixtures are in `python/tests/conftest.py`.

## Build & Test — C++

All commands run from the `cpp/` directory.

```bash
cd cpp/

# Configure (vcpkg toolchain required)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Release

# Run tests
ctest --test-dir build --output-on-failure -C Release
```

Tests live in `cpp/tests/`.

---

## Architecture — Python

```
python/
  src/
    app/         bootstrap.py — DI wiring (_build_services, _build_viewmodels, _connect_signals)
    models/      Pure dataclasses: CalibrationModel, GridModel, GameSessionModel, contracts
    services/    Business logic — one class per concern, all typed against protocols
    viewmodels/  MVVM bridge — QObject subclasses, emit signals, hold no UI code
    views/       PyQt6 widgets — read from viewmodels, forward user actions
    workers/     QThread workers: CameraWorker, VisionWorker, RenderWorker
    utils/       config.py, constants.py, app_paths.py, image_conversion.py, logging.py
  tests/
    conftest.py   Shared fixtures: make_session, make_calibration, make_grid, make_frame_packet
    unit/         One test file per source module
```

## Architecture — C++

```
cpp/
  src/
    app/         application.h/cpp — DI wiring + main loop
    models/      Pure structs: CalibrationModel, GridModel, GameSessionModel, contracts
    services/    Business logic — interfaces in interfaces.h
    threading/   WorkerThread, LatestValue, CameraWorker, VisionWorker
    ui/          Win32+D3D11 windows, Dear ImGui operator panel
    utils/       config, constants, math_helpers, D3D helpers, 2D renderer
  tests/         GTest unit tests
  main.cpp       WinMain entry point
```

### Key Design Rules (Python)

- **Services depend on protocols, not concrete types.** All service interfaces are in
  `python/src/services/protocols.py`. ViewModels accept protocol types — never import a concrete service
  class into a viewmodel constructor directly (bootstrap.py is the one exception).
- **No magic numbers.** All tuning constants live in `python/src/utils/constants.py`.
- **Models are pure dataclasses.** No Qt, no I/O, no service imports inside `python/src/models/`.
- **Workers never hold business logic.** Delegate to services immediately.
- **Bootstrap is the only place that wires concrete types together.**

### Key Design Rules (C++)

- **Services depend on interfaces.** All service interfaces are in `cpp/src/services/interfaces.h`.
- **No magic numbers.** Constants live in `cpp/src/utils/constants.h`.
- **Models are pure structs.** No D3D, no I/O, no service includes inside `cpp/src/models/`.
- **Workers never hold business logic.** Delegate to services immediately.
- **Application is the only place that wires concrete types together.**

---

## Module Responsibilities

| Module                 | Responsibility                                                                 |
| ---------------------- | ------------------------------------------------------------------------------ |
| `CalibrationService`   | ArUco detection, homography + `inverse_homography` computation                 |
| `FloorMappingService`  | Image coords → floor coords via homography                                     |
| `PlayerTrackerService` | Associates pose detections to stable player IDs                                |
| `GameEngineService`    | Round lifecycle state machine; delegates evaluation to `RoundEvaluator`        |
| `RoundEvaluator`       | Pure evaluation: survivors vs eliminated from a mapped-players snapshot        |
| `OverlayRenderService` | AR overlay on camera frame using cached `inverse_homography`                   |
| `DebugRenderService`   | Debug grid canvas — no camera feed required                                    |
| `CameraCoordinator`    | Camera index/profile management, emits `camera_status_changed`                 |
| `ConfigCoordinator`    | Grid/timing/ArUco settings, emits `config_changed`                             |
| `MainViewModel`        | Facade: delegates to coordinators; orchestrates pose → floor → player pipeline |

---

## Testing Conventions

- **One test class per public method or behaviour group**, not per class.
- **Test file naming**: `test_<module_name>.py` mirrors `python/src/.../module_name.py`.
- **Mock at the boundary**: mock `cv2.VideoCapture`, `mediapipe` landmarker, `ConfigStore`; never mock models.
- **Use `pytest-qt`'s `qtbot`** for any test that touches `QObject` signals (`waitSignal`, not `connect + assert`).
- **Use `pytest`-style classes** (no `unittest.TestCase`) for new test files.
- Existing `unittest.TestCase`-style files may be extended in-place without converting.
- Coverage target: **>85% on `python/src/services/` and `python/src/models/`**. Views/workers are excluded from coverage gates.

### Fixture Guide

```python
# conftest.py fixtures available everywhere
make_session()          # fresh GameSessionModel
make_calibration()      # valid CalibrationModel with np.eye(3) homography
make_grid(rows, cols)   # GridModel
make_frame_packet(live) # FramePacket with a black numpy frame
make_app_config()       # AppConfig with defaults
```

---

## Code Style

- `from __future__ import annotations` at top of every file.
- Max line length: **120** (configured in `python/pyproject.toml`).
- Imports sorted by `ruff` (`I` rules) — run `ruff check --fix` before finalising.
- Type annotations required on all new functions and class fields.
- No `Any` unless wrapping an untyped third-party API (mediapipe, cv2); isolate it.

---

## Adding a New Feature — Checklist (Python)

1. [ ] Identify the responsible module (service? viewmodel? model?).
2. [ ] If a new service, add a Protocol to `python/src/services/protocols.py` first.
3. [ ] Write the test file `python/tests/unit/test_<name>.py` — confirm it fails.
4. [ ] Implement; keep public surface minimal.
5. [ ] If a new constant is needed, add it to `python/src/utils/constants.py`.
6. [ ] Wire into bootstrap only if a new concrete type needs constructing.
7. [ ] Run `cd python && python -m pytest tests/ -v` — all 127+ tests must pass.
8. [ ] Run `cd python && ruff check src/ tests/ --select E,F,I,UP,B` — must be clean.

---

## What NOT to Do

- Do not compute `np.linalg.inv(homography)` at render time — use `calibration.inverse_homography`.
- Do not hardcode pixel sizes, durations, distances, or landmark indices — use `constants.py`.
- Do not import concrete service classes into viewmodels; use the protocol types.
- Do not add Qt imports to `python/src/models/` or `python/src/services/`.
- Do not modify `python/src/app/bootstrap.py` to add business logic — it is wiring only.
- Do not skip writing a test because the code "looks simple".
