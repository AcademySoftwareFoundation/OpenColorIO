# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from PySide6 import QtCore


settings = QtCore.QSettings(
    QtCore.QSettings.IniFormat, QtCore.QSettings.UserScope, "OpenColorIO", "ocioview"
)
"""Global application settings."""
