from __future__ import annotations

import numpy as np
from PyQt6.QtGui import QImage


def numpy_to_qimage(image: np.ndarray) -> QImage:
    if image.ndim != 3 or image.shape[2] != 3:
        raise ValueError("Expected an RGB image with shape (height, width, 3)")

    height, width, channels = image.shape
    bytes_per_line = channels * width
    return QImage(image.data, width, height, bytes_per_line, QImage.Format.Format_RGB888).copy()
