# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import logging
import os
import sys
from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from . import log_handlers  # Import to initialize logging
from .style import QSS, DarkPalette


def excepthook(exc_type, exc_value, exc_tb):
    """Log uncaught errors (especially those raised in Qt slots)."""
    if issubclass(exc_type, KeyboardInterrupt):
        sys.__excepthook__(exc_type, exc_value, exc_tb)
        return
    logging.error(f"{exc_value}", exc_info=exc_value)


def setup_excepthook() -> None:
    """Install exception hook."""
    sys.excepthook = excepthook


def setup_opengl() -> None:
    """
    OpenGL core profile is needed on macOS to access programmatic
    pipeline.
    """
    gl_format = QtGui.QSurfaceFormat()
    gl_format.setProfile(QtGui.QSurfaceFormat.CoreProfile)
    gl_format.setSwapInterval(1)
    gl_format.setVersion(4, 0)
    QtGui.QSurfaceFormat.setDefaultFormat(gl_format)


def setup_env() -> None:
    """Clean OCIO environment to isolate working config."""
    for env_var in (
        ocio.OCIO_CONFIG_ENVVAR,
        ocio.OCIO_ACTIVE_VIEWS_ENVVAR,
        ocio.OCIO_ACTIVE_DISPLAYS_ENVVAR,
        ocio.OCIO_INACTIVE_COLORSPACES_ENVVAR,
        ocio.OCIO_OPTIMIZATION_FLAGS_ENVVAR,
        ocio.OCIO_USER_CATEGORIES_ENVVAR,
    ):
        if env_var in os.environ:
            del os.environ[env_var]


def setup_style(app: QtWidgets.QApplication) -> None:
    """Initialize app style."""
    app.setStyle("fusion")
    app.setPalette(DarkPalette())
    app.setStyleSheet(QSS)
    app.setEffectEnabled(QtCore.Qt.UI_AnimateCombo, False)

    font = app.font()
    font.setPointSize(8)
    app.setFont(font)


def setup_app(
    app: Optional[QtWidgets.QApplication] = None,
) -> QtWidgets.QApplication:
    """Create and configure QApplication."""
    # Setup environment
    setup_excepthook()
    setup_opengl()
    setup_env()

    # Create and/or setup app
    if app is None:
        app = QtWidgets.QApplication(sys.argv)
    setup_style(app)

    return app
