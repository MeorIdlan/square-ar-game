from __future__ import annotations

import numpy as np
import pytest

from src.utils.image_conversion import numpy_to_qimage


class TestNumpyToQImage:
    def test_bgr_frame_converts(self) -> None:
        frame = np.zeros((480, 640, 3), dtype=np.uint8)
        frame[0, 0] = [255, 0, 0]
        image = numpy_to_qimage(frame)
        assert image.width() == 640
        assert image.height() == 480

    def test_small_image(self) -> None:
        frame = np.ones((2, 3, 3), dtype=np.uint8) * 128
        image = numpy_to_qimage(frame)
        assert image.width() == 3
        assert image.height() == 2

    def test_grayscale_raises(self) -> None:
        frame = np.zeros((100, 100), dtype=np.uint8)
        with pytest.raises(ValueError, match="Expected an RGB image"):
            numpy_to_qimage(frame)

    def test_four_channel_raises(self) -> None:
        frame = np.zeros((100, 100, 4), dtype=np.uint8)
        with pytest.raises(ValueError, match="Expected an RGB image"):
            numpy_to_qimage(frame)
