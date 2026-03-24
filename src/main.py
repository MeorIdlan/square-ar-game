from __future__ import annotations

from src.app.bootstrap import build_application


def main() -> int:
    context = build_application()
    context.main_window.show()
    context.projector_window.show()
    if context.main_viewmodel.config.display.show_debug_window:
        context.debug_window.show()
    return context.app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
