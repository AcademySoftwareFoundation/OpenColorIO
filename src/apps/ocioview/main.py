# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import logging
import os
import sys
from pathlib import Path

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets, QtOpenGL

import ocioview.log_handlers  # Import to initialize logging
from ocioview.main_window import OCIOView
from ocioview.style import QSS, DarkPalette


ROOT_DIR = Path(__file__).resolve().parent.parent
FONTS_DIR = ROOT_DIR / "fonts"


def excepthook(exc_type, exc_value, exc_tb):
    """Log uncaught errors"""
    if issubclass(exc_type, KeyboardInterrupt):
        sys.__excepthook__(exc_type, exc_value, exc_tb)
        return
    logging.error(f"{exc_value}", exc_info=exc_value)


if __name__ == "__main__":
    sys.excepthook = excepthook

    # OpenGL core profile needed on macOS to access programmatic pipeline
    gl_format = QtGui.QSurfaceFormat()
    gl_format.setProfile(QtGui.QSurfaceFormat.CoreProfile)
    gl_format.setSwapInterval(1)
    gl_format.setVersion(4, 0)
    QtGui.QSurfaceFormat.setDefaultFormat(gl_format)

    # Create app
    app = QtWidgets.QApplication(sys.argv)

    # Initialize style
    app.setStyle("fusion")
    app.setPalette(DarkPalette())
    app.setStyleSheet(QSS)
    app.setEffectEnabled(QtCore.Qt.UI_AnimateCombo, False)

    font = app.font()
    font.setPointSize(8)
    app.setFont(font)

    # Clean OCIO environment to isolate working config
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

    # Start ocioview
    ocioview = OCIOView()
    ocioview.show()

    sys.exit(app.exec_())
