from __future__ import annotations

from PyQt6.QtGui import QGuiApplication
from PyQt6.QtWidgets import (
    QComboBox,
    QDoubleSpinBox,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QLabel,
    QMainWindow,
    QPushButton,
    QSpinBox,
    QStatusBar,
    QTextEdit,
    QWidget,
)

from src.models.game_session_model import GameSessionModel
from src.viewmodels.main_viewmodel import MainViewModel
from src.utils.config import CameraProfile


class MainWindow(QMainWindow):
    def __init__(self, viewmodel: MainViewModel) -> None:
        super().__init__()
        self._viewmodel = viewmodel
        self.setWindowTitle("Square AR Game Control")
        self.resize(720, 480)

        central_widget = QWidget(self)
        layout = QGridLayout(central_widget)

        controls_box = QGroupBox("Controls")
        controls_layout = QFormLayout(controls_box)

        self._rows_spin = QSpinBox()
        self._rows_spin.setRange(2, 12)
        self._rows_spin.setValue(self._viewmodel.config.grid.rows)
        self._rows_spin.valueChanged.connect(self._viewmodel.update_grid_rows)
        controls_layout.addRow("Rows", self._rows_spin)

        self._columns_spin = QSpinBox()
        self._columns_spin.setRange(2, 12)
        self._columns_spin.setValue(self._viewmodel.config.grid.columns)
        self._columns_spin.valueChanged.connect(self._viewmodel.update_grid_columns)
        controls_layout.addRow("Columns", self._columns_spin)

        self._camera_combo = QComboBox()
        self._populate_camera_choices()
        self._camera_combo.currentIndexChanged.connect(self._on_camera_changed)
        controls_layout.addRow("Camera", self._camera_combo)

        refresh_cameras_button = QPushButton("Rescan Cameras")
        refresh_cameras_button.clicked.connect(self._refresh_camera_choices)
        controls_layout.addRow(refresh_cameras_button)

        reconnect_camera_button = QPushButton("Reconnect Camera")
        reconnect_camera_button.clicked.connect(self._reconnect_camera)
        controls_layout.addRow(reconnect_camera_button)

        self._camera_profile_combo = QComboBox()
        self._populate_camera_profile_choices()
        self._camera_profile_combo.currentIndexChanged.connect(self._on_camera_profile_changed)
        controls_layout.addRow("Camera Mode", self._camera_profile_combo)

        refresh_camera_modes_button = QPushButton("Refresh Camera Modes")
        refresh_camera_modes_button.clicked.connect(self._refresh_camera_profile_choices)
        controls_layout.addRow(refresh_camera_modes_button)

        self._display_combo = QComboBox()
        self._populate_display_choices()
        self._display_combo.currentIndexChanged.connect(self._on_display_changed)
        controls_layout.addRow("Projector Display", self._display_combo)

        refresh_displays_button = QPushButton("Refresh Displays")
        refresh_displays_button.clicked.connect(self._refresh_display_choices)
        controls_layout.addRow(refresh_displays_button)

        self._aruco_dictionary_combo = QComboBox()
        self._populate_aruco_dictionary_choices()
        self._aruco_dictionary_combo.currentIndexChanged.connect(self._on_aruco_dictionary_changed)
        controls_layout.addRow("ArUco Dictionary", self._aruco_dictionary_combo)

        self._pose_model_combo = QComboBox()
        self._populate_pose_model_choices()
        self._pose_model_combo.currentIndexChanged.connect(self._on_pose_model_changed)
        controls_layout.addRow("Pose Model", self._pose_model_combo)

        calibrate_button = QPushButton("Calibrate")
        calibrate_button.clicked.connect(self._viewmodel.calibrate)
        controls_layout.addRow(calibrate_button)

        reset_calibration_button = QPushButton("Reset Calibration")
        reset_calibration_button.clicked.connect(self._viewmodel.reset_calibration)
        controls_layout.addRow(reset_calibration_button)

        start_round_button = QPushButton("Start Round")
        start_round_button.clicked.connect(self._viewmodel.start_round)
        controls_layout.addRow(start_round_button)

        lock_round_button = QPushButton("Lock Round")
        lock_round_button.clicked.connect(self._viewmodel.lock_round)
        controls_layout.addRow(lock_round_button)

        next_round_button = QPushButton("Force Next Round")
        next_round_button.clicked.connect(self._viewmodel.force_next_round)
        controls_layout.addRow(next_round_button)

        reset_button = QPushButton("Reset Session")
        reset_button.clicked.connect(self._viewmodel.reset_session)
        controls_layout.addRow(reset_button)

        save_button = QPushButton("Save Settings")
        save_button.clicked.connect(self._viewmodel.save_config)
        controls_layout.addRow(save_button)

        self._status_label = QLabel("Booting")
        controls_layout.addRow("Status", self._status_label)

        self._phase_label = QLabel("IDLE")
        controls_layout.addRow("Phase", self._phase_label)

        self._timer_label = QLabel("0.0")
        controls_layout.addRow("Timer", self._timer_label)

        self._calibration_state_label = QLabel("NOT_CALIBRATED")
        controls_layout.addRow("Calibration", self._calibration_state_label)

        self._camera_health_label = QLabel("Waiting for camera frames")
        self._camera_health_label.setWordWrap(True)
        controls_layout.addRow("Camera Health", self._camera_health_label)

        self._pose_health_label = QLabel("Pose tracking idle")
        self._pose_health_label.setWordWrap(True)
        controls_layout.addRow("Pose Health", self._pose_health_label)

        self._display_health_label = QLabel("Projector not assigned")
        self._display_health_label.setWordWrap(True)
        controls_layout.addRow("Display Health", self._display_health_label)

        self._calibration_markers_label = QLabel("0 visible")
        controls_layout.addRow("Markers", self._calibration_markers_label)

        self._calibration_message_label = QLabel("Not calibrated")
        self._calibration_message_label.setWordWrap(True)
        controls_layout.addRow("Diagnostics", self._calibration_message_label)

        layout.addWidget(controls_box, 0, 0)

        timings_box = QGroupBox("Timing")
        timings_layout = QFormLayout(timings_box)
        self._add_timing_spinbox(timings_layout, "Flashing", "flashing_duration_seconds", self._viewmodel.config.timings.flashing_duration_seconds)
        self._add_timing_spinbox(timings_layout, "Flash Interval", "flash_interval_seconds", self._viewmodel.config.timings.flash_interval_seconds)
        self._add_timing_spinbox(timings_layout, "Reaction", "reaction_window_seconds", self._viewmodel.config.timings.reaction_window_seconds)
        self._add_timing_spinbox(timings_layout, "Check Delay", "lock_delay_seconds", self._viewmodel.config.timings.lock_delay_seconds)
        self._add_timing_spinbox(timings_layout, "Results", "elimination_display_seconds", self._viewmodel.config.timings.elimination_display_seconds)
        self._add_timing_spinbox(timings_layout, "Inter-Round", "inter_round_delay_seconds", self._viewmodel.config.timings.inter_round_delay_seconds)
        self._add_timing_spinbox(timings_layout, "Missing Grace", "missed_detection_grace_seconds", self._viewmodel.config.timings.missed_detection_grace_seconds)
        layout.addWidget(timings_box, 1, 0)

        overrides_box = QGroupBox("Overrides")
        overrides_layout = QFormLayout(overrides_box)
        self._player_combo = QComboBox()
        overrides_layout.addRow("Player", self._player_combo)

        revive_button = QPushButton("Revive")
        revive_button.clicked.connect(self._on_revive_player)
        overrides_layout.addRow(revive_button)

        eliminate_button = QPushButton("Eliminate")
        eliminate_button.clicked.connect(self._on_eliminate_player)
        overrides_layout.addRow(eliminate_button)

        remove_button = QPushButton("Remove")
        remove_button.clicked.connect(self._on_remove_player)
        overrides_layout.addRow(remove_button)
        layout.addWidget(overrides_box, 2, 0)

        self._log_text = QTextEdit()
        self._log_text.setReadOnly(True)
        layout.addWidget(self._log_text, 0, 1, 3, 1)

        self.setCentralWidget(central_widget)

        status_bar = QStatusBar(self)
        self.setStatusBar(status_bar)

        self._viewmodel.status_changed.connect(self._append_status)
        self._viewmodel.session_changed.connect(self._update_session_view)

    def _append_status(self, message: str) -> None:
        self._status_label.setText(message)
        self._log_text.append(message)
        status_bar = self.statusBar()
        if status_bar is not None:
            status_bar.showMessage(message, 5000)

    def _update_session_view(self, session_model: GameSessionModel) -> None:
        self._status_label.setText(session_model.status_message)
        self._phase_label.setText(session_model.round_state.phase.name)
        self._timer_label.setText(f"{session_model.round_state.timer_remaining:0.1f}")
        self._calibration_state_label.setText(session_model.calibration.state.name)
        self._camera_health_label.setText(session_model.camera_status_message)
        self._pose_health_label.setText(session_model.pose_status_message)
        self._display_health_label.setText(session_model.display_status_message)
        visible_ids = ", ".join(str(marker_id) for marker_id in session_model.calibration.detected_marker_ids) or "none"
        missing_ids = ", ".join(str(marker_id) for marker_id in session_model.calibration.missing_marker_ids) or "none"
        self._calibration_markers_label.setText(
            f"{session_model.calibration.detected_marker_count} visible | visible: {visible_ids} | missing: {missing_ids}"
        )
        self._calibration_message_label.setText(session_model.calibration.validation_message)
        current_selection = self._player_combo.currentText()
        player_ids = sorted(session_model.players.keys())
        self._player_combo.blockSignals(True)
        self._player_combo.clear()
        self._player_combo.addItems(player_ids)
        if current_selection in player_ids:
            self._player_combo.setCurrentText(current_selection)
        self._player_combo.blockSignals(False)

    def _add_timing_spinbox(self, layout: QFormLayout, label: str, field_name: str, value: float) -> None:
        spinbox = QDoubleSpinBox()
        spinbox.setRange(0.05, 30.0)
        spinbox.setSingleStep(0.05)
        spinbox.setDecimals(2)
        spinbox.setValue(value)
        spinbox.valueChanged.connect(lambda new_value, name=field_name: self._viewmodel.update_timing_setting(name, new_value))
        layout.addRow(label, spinbox)

    def _selected_player_id(self) -> str | None:
        player_id = self._player_combo.currentText().strip()
        return player_id or None

    def _populate_camera_choices(self) -> None:
        current_index = self._viewmodel.config.camera.camera_index
        self._camera_combo.blockSignals(True)
        self._camera_combo.clear()
        for camera_index in self._viewmodel.available_camera_indices():
            self._camera_combo.addItem(f"Camera {camera_index}", camera_index)
        selected_index = self._camera_combo.findData(current_index)
        if selected_index >= 0:
            self._camera_combo.setCurrentIndex(selected_index)
        self._camera_combo.blockSignals(False)

    def _populate_camera_profile_choices(self, probe: bool = False) -> None:
        current_profile = self._viewmodel.config.camera.profile
        self._camera_profile_combo.blockSignals(True)
        self._camera_profile_combo.clear()
        for profile in self._viewmodel.available_camera_profiles(probe=probe):
            self._camera_profile_combo.addItem(profile.label, profile)
        selected_index = self._find_camera_profile_index(current_profile)
        if selected_index >= 0:
            self._camera_profile_combo.setCurrentIndex(selected_index)
        self._camera_profile_combo.blockSignals(False)

    def _find_camera_profile_index(self, profile: CameraProfile) -> int:
        for index in range(self._camera_profile_combo.count()):
            item = self._camera_profile_combo.itemData(index)
            if isinstance(item, CameraProfile) and item == profile:
                return index
        return -1

    def _populate_display_choices(self) -> None:
        current_index = self._viewmodel.config.display.projector_screen_index
        screen_count = len(QGuiApplication.screens())
        self._display_combo.blockSignals(True)
        self._display_combo.clear()
        for screen_index, screen in enumerate(QGuiApplication.screens()):
            geometry = screen.availableGeometry()
            label = f"Display {screen_index} ({geometry.width()}x{geometry.height()})"
            self._display_combo.addItem(label, screen_index)
        selected_index = self._display_combo.findData(current_index)
        if selected_index >= 0:
            self._display_combo.setCurrentIndex(selected_index)
        self._display_combo.blockSignals(False)
        self._viewmodel.update_display_probe(screen_count)
        if hasattr(self, "_display_health_label"):
            self._display_health_label.setText(
                f"Projector assigned to display {current_index} | {screen_count} display(s) detected"
            )

    def _populate_aruco_dictionary_choices(self) -> None:
        current_value = self._viewmodel.config.aruco_dictionary
        self._aruco_dictionary_combo.blockSignals(True)
        self._aruco_dictionary_combo.clear()
        for dictionary_name in self._viewmodel.supported_aruco_dictionaries():
            self._aruco_dictionary_combo.addItem(dictionary_name, dictionary_name)
        selected_index = self._aruco_dictionary_combo.findData(current_value)
        if selected_index >= 0:
            self._aruco_dictionary_combo.setCurrentIndex(selected_index)
        self._aruco_dictionary_combo.blockSignals(False)

    def _populate_pose_model_choices(self) -> None:
        current_value = self._viewmodel.current_pose_model_path()
        self._pose_model_combo.blockSignals(True)
        self._pose_model_combo.clear()
        for label, model_path in self._viewmodel.supported_pose_models():
            self._pose_model_combo.addItem(label, model_path)
        selected_index = self._pose_model_combo.findData(current_value)
        if selected_index >= 0:
            self._pose_model_combo.setCurrentIndex(selected_index)
        self._pose_model_combo.blockSignals(False)

    def _on_camera_changed(self, combo_index: int) -> None:
        if combo_index < 0:
            return
        camera_index = self._camera_combo.itemData(combo_index)
        if isinstance(camera_index, int):
            self._viewmodel.update_camera_index(camera_index)
            self._populate_camera_profile_choices()

    def _on_camera_profile_changed(self, combo_index: int) -> None:
        if combo_index < 0:
            return
        profile = self._camera_profile_combo.itemData(combo_index)
        if isinstance(profile, CameraProfile):
            self._viewmodel.update_camera_profile(profile)

    def _on_display_changed(self, combo_index: int) -> None:
        if combo_index < 0:
            return
        screen_index = self._display_combo.itemData(combo_index)
        if isinstance(screen_index, int):
            self._viewmodel.update_projector_screen_index(screen_index)

    def _on_aruco_dictionary_changed(self, combo_index: int) -> None:
        if combo_index < 0:
            return
        dictionary_name = self._aruco_dictionary_combo.itemData(combo_index)
        if isinstance(dictionary_name, str):
            self._viewmodel.update_aruco_dictionary(dictionary_name)

    def _on_pose_model_changed(self, combo_index: int) -> None:
        if combo_index < 0:
            return
        model_path = self._pose_model_combo.itemData(combo_index)
        if isinstance(model_path, str):
            self._viewmodel.update_pose_model(model_path)

    def _refresh_camera_choices(self) -> None:
        self._viewmodel.refresh_camera_sources()
        self._populate_camera_choices()
        self._populate_camera_profile_choices()

    def _refresh_camera_profile_choices(self) -> None:
        self._populate_camera_profile_choices(probe=True)

    def _reconnect_camera(self) -> None:
        self._viewmodel.reconnect_camera()

    def _refresh_display_choices(self) -> None:
        self._populate_display_choices()

    def _on_revive_player(self) -> None:
        player_id = self._selected_player_id()
        if player_id is not None:
            self._viewmodel.revive_player(player_id)

    def _on_eliminate_player(self) -> None:
        player_id = self._selected_player_id()
        if player_id is not None:
            self._viewmodel.eliminate_player(player_id)

    def _on_remove_player(self) -> None:
        player_id = self._selected_player_id()
        if player_id is not None:
            self._viewmodel.remove_player(player_id)
