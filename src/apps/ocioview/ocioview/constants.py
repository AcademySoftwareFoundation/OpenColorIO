# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from pathlib import Path

from PySide6 import QtCore, QtGui


# Root application directory
ROOT_DIR = Path(__file__).parent.parent

# Sizes
ICON_SIZE_ITEM = QtCore.QSize(20, 20)
ICON_SIZE_BUTTON = QtCore.QSize(24, 24)
ICON_SCALE_FACTOR = 1.15

MARGIN_WIDTH = 13  # Pixels

# Colors
BORDER_COLOR_ROLE = QtGui.QPalette.Dark
TOOL_BAR_BG_COLOR_ROLE = QtGui.QPalette.Mid
TOOL_BAR_BORDER_COLOR_ROLE = QtGui.QPalette.Midlight

GRAY_COLOR = QtGui.QColor("dimgray")
R_COLOR = QtGui.QColor.fromHsvF(0.0, 0.5, 1.0)
G_COLOR = QtGui.QColor.fromHsvF(0.33, 0.5, 1.0)
B_COLOR = QtGui.QColor.fromHsvF(0.66, 0.5, 1.0)

# Icons
ICONS_DIR = ROOT_DIR / "icons"
ICON_PATH_OCIO = ICONS_DIR / "opencolorio-icon-color.svg"

# Value edit array component label sets
RGB = ("r", "g", "b")
RGBA = tuple(list(RGB) + ["a"])
