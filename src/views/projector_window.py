from __future__ import annotations

from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QCloseEvent, QGuiApplication, QImage, QPixmap
from PyQt6.QtWidgets import QLabel, QVBoxLayout, QWidget


class ProjectorWindow(QWidget):
    close_requested = pyqtSignal()

    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Projector View")
        self.setMinimumSize(960, 540)
        self._suppress_close_request = False

        self._image_label = QLabel("No projector frame yet")
        self._image_label.setAlignment(Qt.AlignmentFlag.AlignCenter)

        layout = QVBoxLayout(self)
        layout.addWidget(self._image_label)

    def set_image(self, image: QImage) -> None:
        pixmap = QPixmap.fromImage(image)
        self._image_label.setPixmap(
            pixmap.scaled(
                self._image_label.size(),
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.FastTransformation,
            )
        )

    def suppress_close_request(self, suppress: bool) -> None:
        self._suppress_close_request = suppress

    def set_target_screen(self, screen_index: int) -> None:
        screens = QGuiApplication.screens()
        if not screens:
            return

        bounded_index = max(0, min(screen_index, len(screens) - 1))
        screen = screens[bounded_index]
        if self.windowHandle() is not None:
            self.windowHandle().setScreen(screen)
        geometry = screen.availableGeometry()
        self.move(geometry.topLeft())
        self.showMaximized()

    def closeEvent(self, event: QCloseEvent) -> None:
        if not self._suppress_close_request:
            self.close_requested.emit()
        super().closeEvent(event)
