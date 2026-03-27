from __future__ import annotations

from dataclasses import dataclass
import logging
from pathlib import Path
from typing import cast

from PyQt6.QtCore import QMetaObject, QThread, Qt
from PyQt6.QtWidgets import QApplication

from src.models.game_session_model import GameSessionModel
from src.services.calibration_service import CalibrationService
from src.services.camera_service import CameraService
from src.services.debug_render_service import DebugRenderService
from src.services.floor_mapping_service import FloorMappingService
from src.services.game_engine_service import GameEngineService
from src.services.overlay_render_service import OverlayRenderService
from src.services.player_tracker_service import PlayerTrackerService
from src.services.pose_tracking_service import PoseTrackingService
from src.utils.app_paths import application_root
from src.utils.config import ConfigStore
from src.utils.logging import configure_logging
from src.viewmodels.calibration_viewmodel import CalibrationViewModel
from src.viewmodels.debug_viewmodel import DebugViewModel
from src.viewmodels.game_viewmodel import GameViewModel
from src.viewmodels.main_viewmodel import MainViewModel
from src.viewmodels.projector_viewmodel import ProjectorViewModel
from src.views.debug_window import DebugWindow
from src.views.main_window import MainWindow
from src.views.projector_window import ProjectorWindow
from src.workers.camera_worker import CameraWorker
from src.workers.render_worker import DebugRenderWorker, ProjectorRenderWorker
from src.workers.vision_worker import VisionWorker


@dataclass(slots=True)
class BootstrapContext:
    app: QApplication
    main_window: MainWindow
    projector_window: ProjectorWindow
    debug_window: DebugWindow
    main_viewmodel: MainViewModel
    camera_worker: CameraWorker
    vision_worker: VisionWorker
    camera_thread: QThread
    vision_thread: QThread
    projector_render_thread: QThread
    debug_render_thread: QThread


def build_application() -> BootstrapContext:
    log_file_path = configure_logging()
    logger = logging.getLogger(__name__)
    existing_app = QApplication.instance()
    if isinstance(existing_app, QApplication):
        app = cast(QApplication, existing_app)
    else:
        app = QApplication([])
    project_root = application_root()
    logger.info("Building application from root %s", project_root)
    logger.info("Active log file: %s", log_file_path)

    config_store = ConfigStore()
    config = config_store.load()
    logger.info(
        "Configuration loaded: camera=%s display=%s aruco=%s pose_model=%s",
        config.camera.camera_index,
        config.display.projector_screen_index,
        config.aruco_dictionary,
        config.pose.model_asset_path,
    )
    session_model = GameSessionModel()
    session_model.grid.rows = config.grid.rows
    session_model.grid.columns = config.grid.columns
    session_model.grid.playable_width = config.grid.playable_width
    session_model.grid.playable_height = config.grid.playable_height
    session_model.grid.reset_states()

    camera_service = CameraService(config.camera)
    calibration_service = CalibrationService(config)
    pose_model_path = Path(config.pose.model_asset_path)
    if not pose_model_path.is_absolute():
        pose_model_path = project_root / pose_model_path
    logger.info("Resolved pose model path to %s", pose_model_path)
    pose_tracking_service = PoseTrackingService(config.pose, pose_model_path)
    floor_mapping_service = FloorMappingService()
    game_engine_service = GameEngineService()
    player_tracker_service = PlayerTrackerService()
    overlay_render_service = OverlayRenderService()
    debug_render_service = DebugRenderService()

    projector_viewmodel = ProjectorViewModel(overlay_render_service)
    debug_viewmodel = DebugViewModel(debug_render_service)
    calibration_viewmodel = CalibrationViewModel(calibration_service, session_model.calibration)
    game_viewmodel = GameViewModel(session_model, game_engine_service)
    main_viewmodel = MainViewModel(
        config=config,
        config_store=config_store,
        session_model=session_model,
        calibration_viewmodel=calibration_viewmodel,
        game_viewmodel=game_viewmodel,
        projector_viewmodel=projector_viewmodel,
        debug_viewmodel=debug_viewmodel,
        camera_service=camera_service,
        pose_tracking_service=pose_tracking_service,
        floor_mapping_service=floor_mapping_service,
        game_engine_service=game_engine_service,
        player_tracker_service=player_tracker_service,
    )

    main_window = MainWindow(main_viewmodel)
    projector_window = ProjectorWindow()
    debug_window = DebugWindow()

    shutting_down = {"active": False}

    def close_primary_windows() -> None:
        if shutting_down["active"]:
            return
        shutting_down["active"] = True
        main_window.suppress_close_request(True)
        projector_window.suppress_close_request(True)
        debug_window.close()
        projector_window.close()
        main_window.close()
        app.quit()

    main_window.close_requested.connect(close_primary_windows)
    projector_window.close_requested.connect(close_primary_windows)

    projector_viewmodel.image_updated.connect(projector_window.set_image)
    debug_viewmodel.image_updated.connect(debug_window.set_image)
    main_viewmodel.projector_screen_changed.connect(projector_window.set_target_screen)

    target_interval_ms = max(1, int(1000 / max(config.camera.target_fps, 1)))

    camera_worker = CameraWorker(camera_service, initial_interval_ms=target_interval_ms)
    camera_thread = QThread()
    camera_worker.moveToThread(camera_thread)
    vision_worker = VisionWorker(pose_tracking_service)
    vision_thread = QThread()
    vision_worker.moveToThread(vision_thread)

    projector_render_worker = ProjectorRenderWorker(overlay_render_service)
    projector_render_thread = QThread()
    projector_render_worker.moveToThread(projector_render_thread)

    debug_render_worker = DebugRenderWorker(debug_render_service)
    debug_render_thread = QThread()
    debug_render_worker.moveToThread(debug_render_thread)

    projector_viewmodel.render_requested.connect(projector_render_worker.render_latest)
    projector_render_worker.image_ready.connect(projector_viewmodel.forward_rendered_image)
    debug_viewmodel.render_requested.connect(debug_render_worker.render_latest)
    debug_render_worker.image_ready.connect(debug_viewmodel.forward_rendered_image)
    camera_thread.started.connect(camera_worker.start)

    camera_thread.start()
    vision_thread.start()
    projector_render_thread.start()
    debug_render_thread.start()

    camera_worker.frame_ready.connect(main_viewmodel.handle_frame_packet)
    camera_worker.frame_ready.connect(vision_worker.process_frame)
    vision_worker.pose_ready.connect(main_viewmodel.handle_pose_result)
    main_viewmodel.camera_capture_interval_changed.connect(camera_worker.set_interval)
    camera_thread.finished.connect(camera_worker.deleteLater)
    vision_thread.finished.connect(vision_worker.deleteLater)
    projector_render_thread.finished.connect(projector_render_worker.deleteLater)
    debug_render_thread.finished.connect(debug_render_worker.deleteLater)

    shutdown_state = {"complete": False}

    def shutdown_workers() -> None:
        if shutdown_state["complete"]:
            return
        shutdown_state["complete"] = True

        main_viewmodel.autosave_config()
        QMetaObject.invokeMethod(
            camera_worker,
            "stop",
            Qt.ConnectionType.BlockingQueuedConnection,
        )
        camera_service.release()
        pose_tracking_service.close()

        camera_thread.quit()
        camera_thread.wait(2000)
        vision_thread.quit()
        vision_thread.wait(2000)
        projector_render_thread.quit()
        projector_render_thread.wait(2000)
        debug_render_thread.quit()
        debug_render_thread.wait(2000)

    app.aboutToQuit.connect(shutdown_workers)

    main_viewmodel.initialize()
    logger.info("Camera worker started at target fps=%s", config.camera.target_fps)

    return BootstrapContext(
        app=app,
        main_window=main_window,
        projector_window=projector_window,
        debug_window=debug_window,
        main_viewmodel=main_viewmodel,
        camera_worker=camera_worker,
        vision_worker=vision_worker,
        camera_thread=camera_thread,
        vision_thread=vision_thread,
        projector_render_thread=projector_render_thread,
        debug_render_thread=debug_render_thread,
    )
