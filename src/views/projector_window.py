from __future__ import annotations

from PyQt6.QtCore import Qt
from PyQt6.QtGui import QGuiApplication, QImage, QPixmap
from PyQt6.QtWidgets import QLabel, QVBoxLayout, QWidget


class ProjectorWindow(QWidget):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Projector View")
        self.setMinimumSize(960, 540)

        self._image_label = QLabel("No projector frame yet")
        self._image_label.setAlignment(Qt.AlignmentFlag.AlignCenter)

        layout = QVBoxLayout(self)
        layout.addWidget(self._image_label)

    def set_image(self, image: QImage) -> None:
        pixmap = QPixmap.fromImage(image)
        self._image_label.setPixmap(pixmap.scaled(self._image_label.size(), Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation))

    def set_target_screen(self, screen_index: int) -> None:
        screens = QGuiApplication.screens()
        if not screens:
            return

        bounded_index = max(0, min(screen_index, len(screens) - 1))
        screen = screens[bounded_index]
        self.windowHandle().setScreen(screen) if self.windowHandle() is not None else None
        geometry = screen.availableGeometry()
        self.setGeometry(geometry)
        self.showFullScreen()
