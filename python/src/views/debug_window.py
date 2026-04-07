from __future__ import annotations

from PyQt6.QtCore import Qt
from PyQt6.QtGui import QImage, QPixmap
from PyQt6.QtWidgets import QLabel, QVBoxLayout, QWidget


class DebugWindow(QWidget):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Debug View")
        self.resize(720, 720)

        self._image_label = QLabel("No debug frame yet")
        self._image_label.setAlignment(Qt.AlignmentFlag.AlignCenter)

        layout = QVBoxLayout(self)
        layout.addWidget(self._image_label)

    def set_image(self, image: QImage) -> None:
        pixmap = QPixmap.fromImage(image)
        self._image_label.setPixmap(pixmap.scaled(self._image_label.size(), Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation))
