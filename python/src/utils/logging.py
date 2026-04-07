from __future__ import annotations

import logging
from logging.handlers import RotatingFileHandler
from pathlib import Path
import sys

from src.utils.app_paths import log_directory


_LOG_FORMAT = "%(asctime)s %(levelname)s [%(name)s] %(message)s"


def configure_logging() -> Path:
    root_logger = logging.getLogger()
    if getattr(configure_logging, "_configured", False):
        return getattr(configure_logging, "_log_file_path")

    root_logger.setLevel(logging.INFO)
    root_logger.handlers.clear()

    formatter = logging.Formatter(_LOG_FORMAT)

    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.INFO)
    console_handler.setFormatter(formatter)
    root_logger.addHandler(console_handler)

    log_file_path = log_directory() / "square-ar-game.log"
    file_handler = RotatingFileHandler(
        log_file_path,
        maxBytes=1_000_000,
        backupCount=5,
        encoding="utf-8",
    )
    file_handler.setLevel(logging.INFO)
    file_handler.setFormatter(formatter)
    root_logger.addHandler(file_handler)

    def _handle_uncaught_exception(exc_type, exc_value, exc_traceback) -> None:
        if issubclass(exc_type, KeyboardInterrupt):
            sys.__excepthook__(exc_type, exc_value, exc_traceback)
            return
        logging.getLogger("square_ar_game.crash").critical(
            "Uncaught exception",
            exc_info=(exc_type, exc_value, exc_traceback),
        )

    sys.excepthook = _handle_uncaught_exception
    configure_logging._configured = True
    configure_logging._log_file_path = log_file_path
    logging.getLogger(__name__).info("File logging initialized at %s", log_file_path)
    return log_file_path
