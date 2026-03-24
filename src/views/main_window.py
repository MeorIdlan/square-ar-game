from __future__ import annotations

from PyQt6.QtWidgets import (
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
        self._rows_spin.valueChanged.connect(self._on_rows_changed)
        controls_layout.addRow("Rows", self._rows_spin)

        self._columns_spin = QSpinBox()
        self._columns_spin.setRange(2, 12)
        self._columns_spin.setValue(self._viewmodel.config.grid.columns)
        self._columns_spin.valueChanged.connect(self._on_columns_changed)
        controls_layout.addRow("Columns", self._columns_spin)

        calibrate_button = QPushButton("Calibrate")
        calibrate_button.clicked.connect(self._viewmodel.calibrate)
        controls_layout.addRow(calibrate_button)

        start_round_button = QPushButton("Start Round")
        start_round_button.clicked.connect(self._viewmodel.start_round)
        controls_layout.addRow(start_round_button)

        lock_round_button = QPushButton("Lock Round")
        lock_round_button.clicked.connect(self._viewmodel.lock_round)
        controls_layout.addRow(lock_round_button)

        save_button = QPushButton("Save Settings")
        save_button.clicked.connect(self._viewmodel.save_config)
        controls_layout.addRow(save_button)

        self._status_label = QLabel("Booting")
        controls_layout.addRow("Status", self._status_label)

        layout.addWidget(controls_box, 0, 0)

        self._log_text = QTextEdit()
        self._log_text.setReadOnly(True)
        layout.addWidget(self._log_text, 0, 1)

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

    def _on_rows_changed(self, value: int) -> None:
        self._viewmodel.config.grid.rows = value
        self._viewmodel.session_model.grid.rows = value
        self._viewmodel.session_model.grid.reset_states()
        self._viewmodel.session_changed.emit(self._viewmodel.session_model)

    def _on_columns_changed(self, value: int) -> None:
        self._viewmodel.config.grid.columns = value
        self._viewmodel.session_model.grid.columns = value
        self._viewmodel.session_model.grid.reset_states()
        self._viewmodel.session_changed.emit(self._viewmodel.session_model)
