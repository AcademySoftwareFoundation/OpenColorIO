# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from PySide6 import QtGui, QtWidgets

from .constants import (
    BORDER_COLOR_ROLE,
    TOOL_BAR_BORDER_COLOR_ROLE,
    TOOL_BAR_BG_COLOR_ROLE,
)


# Application style sheet overrides
QSS = """
"""


class DarkPalette(QtGui.QPalette):
    def __init__(self):
        super().__init__()

        # Central roles
        self.setColor(QtGui.QPalette.Window, QtGui.QColor(60, 60, 60))
        self.setColor(QtGui.QPalette.WindowText, QtGui.QColor(190, 190, 190))
        self.setColor(QtGui.QPalette.Base, QtGui.QColor(50, 50, 50))
        self.setColor(QtGui.QPalette.AlternateBase, QtGui.QColor(45, 45, 45))
        self.setColor(QtGui.QPalette.ToolTipBase, QtGui.QColor(45, 45, 45))
        self.setColor(QtGui.QPalette.ToolTipText, QtGui.QColor(190, 190, 190))
        self.setColor(QtGui.QPalette.PlaceholderText, QtGui.QColor(100, 100, 100))
        self.setColor(QtGui.QPalette.Text, QtGui.QColor(190, 190, 190))
        self.setColor(QtGui.QPalette.Button, QtGui.QColor(50, 50, 50))
        self.setColor(QtGui.QPalette.ButtonText, QtGui.QColor(190, 190, 190))
        self.setColor(QtGui.QPalette.BrightText, QtGui.QColor(190, 190, 190))

        # 3D effects
        self.setColor(QtGui.QPalette.Dark, QtGui.QColor(47, 47, 47))
        self.setColor(QtGui.QPalette.Mid, QtGui.QColor(64, 64, 64))
        self.setColor(QtGui.QPalette.Midlight, QtGui.QColor(81, 81, 81))

        # Highlight
        self.setColor(QtGui.QPalette.Highlight, QtGui.QColor(204, 30, 104))
        self.setColor(QtGui.QPalette.HighlightedText, QtGui.QColor(190, 190, 190))

        # Disabled
        disabled = QtGui.QPalette.Disabled
        self.setColor(disabled, QtGui.QPalette.Base, QtGui.QColor(57, 57, 57))
        self.setColor(disabled, QtGui.QPalette.Text, QtGui.QColor(100, 100, 100))
        self.setColor(disabled, QtGui.QPalette.Button, QtGui.QColor(57, 57, 57))
        self.setColor(disabled, QtGui.QPalette.ButtonText, QtGui.QColor(115, 115, 115))


def apply_top_tool_bar_style(
    widget: QtWidgets.QWidget,
    bg_color_role: Optional[QtGui.QPalette.ColorRole] = TOOL_BAR_BG_COLOR_ROLE,
    border_color_role: QtGui.QPalette.ColorRole = TOOL_BAR_BORDER_COLOR_ROLE,
    border_bottom_radius: int = 0,
):
    """
    Applies a style to the provided widget which wraps it in a styled
    box with rounded top corners, intended as a toolbar visually
    attached to the widget below it.

    .. note::
        The supplied widget MUST have a unique object name for the
        style to apply correctly.

    :param widget: Widget to style
    :param bg_color_role: Optional BG QPalette color role. If not
        specified the current BG color is preserved.
    :param border_color_role: Border QPalette color role
    :param border_bottom_radius: Corner radius for bottom of toolbar
    """
    palette = widget.palette()
    border_color = palette.color(border_color_role).name()

    qss = f"QFrame#{widget.objectName()} {{"
    if bg_color_role is not None:
        qss += f"    background-color: {palette.color(bg_color_role).name()};"
    qss += (
        f"    border-top: 1px solid {border_color};"
        f"    border-right: 1px solid {border_color};"
        f"    border-left: 1px solid {border_color};"
        f"    border-top-left-radius: 3px;"
        f"    border-top-right-radius: 3px;"
        f"    border-bottom-left-radius: {border_bottom_radius:d}px;"
        f"    border-bottom-right-radius: {border_bottom_radius:d}px;"
        f"}}"
    )
    widget.setStyleSheet(qss)


def apply_widget_with_top_tool_bar_style(
    widget: QtWidgets.QWidget,
    border_color_role: QtGui.QPalette.ColorRole = BORDER_COLOR_ROLE,
):
    """
    Applies a style to the provided widget which wraps it in a styled
    box with rounded corners, intended to visually tie together a
    widget with its styled top toolbar.

    .. note::
        The supplied widget MUST have a unique object name for the
        style to apply correctly.

    :param widget: Widget to style
    :param border_color_role: Border QPalette color role
    """
    palette = widget.palette()

    widget.setStyleSheet(
        f"QFrame#base_log_view__log_inner_frame {{"
        f"    border: 1px solid {palette.color(border_color_role).name()};"
        f"    border-radius: 3px;"
        f"}}"
    )
