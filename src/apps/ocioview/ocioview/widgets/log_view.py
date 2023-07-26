# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide2 import QtCore, QtGui, QtWidgets

from ..constants import (
    ICON_SIZE_ITEM,
    TOOL_BAR_BG_COLOR_ROLE,
    TOOL_BAR_BORDER_COLOR_ROLE,
)
from ..logging import LogRouter
from ..utils import get_glyph_icon
from .combo_box import ComboBox
from .text_edit import HtmlView


class LogView(QtWidgets.QWidget):
    """
    Widget for viewing OCIO and application logs.
    """

    @classmethod
    def label(cls) -> str:
        return "Log"

    @classmethod
    def icon(cls) -> QtGui.QIcon:
        return get_glyph_icon("ph.terminal-window")

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        tool_bar_bg_color = self.palette().color(TOOL_BAR_BG_COLOR_ROLE).name()
        tool_bar_border_color = self.palette().color(TOOL_BAR_BORDER_COLOR_ROLE).name()

        # Widgets
        self.log_level_box = ComboBox()
        self.log_level_box.setSizeAdjustPolicy(QtWidgets.QComboBox.AdjustToContents)
        self.log_level_box.addItem("Warning", userData=ocio.LOGGING_LEVEL_WARNING)
        self.log_level_box.addItem("Info", userData=ocio.LOGGING_LEVEL_INFO)
        self.log_level_box.addItem("Debug", userData=ocio.LOGGING_LEVEL_DEBUG)
        self.log_level_box.setCurrentText(
            ocio.LoggingLevelToString(ocio.GetLoggingLevel()).capitalize()
        )

        self.clear_button = QtWidgets.QToolButton()
        self.clear_button.setIcon(get_glyph_icon("mdi6.delete-outline"))

        source_font = QtGui.QFont("Courier")
        source_font.setPointSize(10)

        self.log_view = HtmlView()
        self.log_view.setFont(source_font)

        # Layout
        # TODO: Make this tool bar styling implementation generic, since it is used by
        #       multiple widgets.
        tool_bar_stretch = QtWidgets.QFrame()
        tool_bar_stretch.setSizePolicy(
            QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed
        )

        tool_bar = QtWidgets.QToolBar()
        tool_bar.setContentsMargins(0, 0, 0, 0)
        tool_bar.setIconSize(ICON_SIZE_ITEM)
        tool_bar.addWidget(self.log_level_box)
        tool_bar.addWidget(tool_bar_stretch)
        tool_bar.addWidget(self.clear_button)

        tool_bar_layout = QtWidgets.QVBoxLayout()
        tool_bar_layout.setContentsMargins(0, 0, 0, 0)
        tool_bar_layout.addWidget(tool_bar)

        tool_bar_frame = QtWidgets.QFrame()
        tool_bar_frame.setFrameShape(QtWidgets.QFrame.StyledPanel)
        tool_bar_frame.setObjectName("log_view__tool_bar_frame")
        tool_bar_frame.setStyleSheet(
            f"QFrame#log_view__tool_bar_frame {{"
            f"    background-color: {tool_bar_bg_color};"
            f"    border-top: 1px solid {tool_bar_border_color};"
            f"    border-right: 1px solid {tool_bar_border_color};"
            f"    border-left: 1px solid {tool_bar_border_color};"
            f"    border-top-left-radius: 3px;"
            f"    border-top-right-radius: 3px;"
            f"    border-bottom-left-radius: 0px;"
            f"    border-bottom-right-radius: 0px;"
            f"}}"
        )
        tool_bar_frame.setLayout(tool_bar_layout)

        log_inner_layout = QtWidgets.QVBoxLayout()
        log_inner_layout.setContentsMargins(0, 0, 0, 0)
        log_inner_layout.setSpacing(1)
        log_inner_layout.addWidget(tool_bar_frame)
        log_inner_layout.addWidget(self.log_view)

        log_inner_frame = QtWidgets.QFrame()
        log_inner_frame.setFrameShape(QtWidgets.QFrame.StyledPanel)
        log_inner_frame.setObjectName("log_view__log_inner_frame")
        log_inner_frame.setStyleSheet(
            f"QFrame#log_view__log_inner_frame {{"
            f"    border: 1px solid "
            f"        {self.palette().color(QtGui.QPalette.Dark).name()};"
            f"    border-radius: 3px;"
            f"}}"
        )
        log_inner_frame.setLayout(log_inner_layout)

        log_layout = QtWidgets.QVBoxLayout()
        log_layout.addWidget(log_inner_frame)
        log_frame = QtWidgets.QFrame()
        log_frame.setLayout(log_layout)

        self.tabs = QtWidgets.QTabWidget()
        self.tabs.addTab(log_frame, self.icon(), self.label())

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.tabs)
        self.setLayout(layout)

        # Initialize
        self.log_level_box.currentIndexChanged[int].connect(self._on_log_level_changed)
        self.clear_button.released.connect(self.reset)

        log_router = LogRouter.get_instance()
        log_router.error_logged.connect(self._on_record_logged)
        log_router.warning_logged.connect(self._on_record_logged)
        log_router.info_logged.connect(self._on_record_logged)
        log_router.debug_logged.connect(self._on_record_logged)

    def reset(self) -> None:
        """Clear log history."""
        self.log_view.reset()

    @QtCore.Slot(int)
    def _on_log_level_changed(self, index: int):
        """Update global logging level."""
        log_level = self.log_level_box.currentData()
        LogRouter.set_logging_level(log_level)

    @QtCore.Slot(str)
    def _on_record_logged(self, record: str) -> None:
        """Append record to general log view."""
        self.log_view.append(record)
