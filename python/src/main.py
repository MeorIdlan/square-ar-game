from __future__ import annotations

import logging

from src.app.bootstrap import build_application


def main() -> int:
    context = build_application()
    logging.getLogger(__name__).info("Application windows constructed; entering Qt event loop")
    context.main_window.show()
    context.projector_window.show()
    if context.main_viewmodel.config.display.show_debug_window:
        context.debug_window.show()
    return context.app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
