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
- worker scaffolding for future camera and pose threads

MediaPipe multi-player tracking and projector compositing over the live camera feed are still placeholder at this stage.

## Run

Create and activate a Python environment, install the dependencies, then run:

```bash
python -m src.main
```

## Next implementation targets

1. Replace placeholder calibration with four-marker ArUco detection and homography validation.
2. Replace placeholder pose tracking with MediaPipe lower-body extraction.
3. Feed the live camera frame into projector rendering.
4. Add round timers and operator override actions.
