from __future__ import annotations

from PyQt6.QtCore import Qt
from PyQt6.QtGui import QImage, QPixmap
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
