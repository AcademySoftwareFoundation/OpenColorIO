# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Any, Optional

import PyOpenColorIO as ocio
from PySide2 import QtCore, QtGui, QtWidgets

from ..constants import (
    ICON_SIZE_BUTTON,
    TOOL_BAR_BG_COLOR_ROLE,
    TOOL_BAR_BORDER_COLOR_ROLE,
)
from ..utils import get_glyph_icon
from ..widgets import ComboBox, HtmlView
from .log_router import LogRouter


class LogView(QtWidgets.QFrame):
    """Base widget for a log/code viewer and toolbar."""

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        tool_bar_bg_color = self.palette().color(TOOL_BAR_BG_COLOR_ROLE).name()
        tool_bar_border_color = self.palette().color(TOOL_BAR_BORDER_COLOR_ROLE).name()

        source_font = QtGui.QFont("Courier")
        source_font.setPointSize(10)

        # Widgets
        self.html_view = HtmlView()
        self.html_view.setFont(source_font)

        # Layout
        tool_bar_stretch = QtWidgets.QFrame()
        tool_bar_stretch.setSizePolicy(
            QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed
        )

        self.tool_bar = QtWidgets.QToolBar()
        self.tool_bar.setContentsMargins(0, 0, 0, 0)
        self.tool_bar.setIconSize(ICON_SIZE_BUTTON)
        self.tool_bar.addWidget(tool_bar_stretch)

        tool_bar_layout = QtWidgets.QVBoxLayout()
        tool_bar_layout.setContentsMargins(0, 0, 0, 0)
        tool_bar_layout.addWidget(self.tool_bar)

        # TODO: Make tool bar styling implementation generic, since it is used by
        #       multiple widgets.
        tool_bar_frame = QtWidgets.QFrame()
        tool_bar_frame.setFrameShape(QtWidgets.QFrame.StyledPanel)
        tool_bar_frame.setObjectName("base_log_view__tool_bar_frame")
        tool_bar_frame.setStyleSheet(
            f"QFrame#base_log_view__tool_bar_frame {{"
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

        inner_layout = QtWidgets.QVBoxLayout()
        inner_layout.setContentsMargins(0, 0, 0, 0)
        inner_layout.setSpacing(1)
        inner_layout.addWidget(tool_bar_frame)
        inner_layout.addWidget(self.html_view)

        inner_frame = QtWidgets.QFrame()
        inner_frame.setFrameShape(QtWidgets.QFrame.StyledPanel)
        inner_frame.setObjectName("base_log_view__log_inner_frame")
        inner_frame.setStyleSheet(
            f"QFrame#base_log_view__log_inner_frame {{"
            f"    border: 1px solid "
            f"        {self.palette().color(QtGui.QPalette.Dark).name()};"
            f"    border-radius: 3px;"
            f"}}"
        )
        inner_frame.setLayout(inner_layout)

        outer_layout = QtWidgets.QVBoxLayout()
        outer_layout.setContentsMargins(0, 0, 0, 0)
        outer_layout.addWidget(inner_frame)

        outer_frame = QtWidgets.QFrame()
        outer_frame.setLayout(outer_layout)

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(outer_frame)
        self.setLayout(layout)

    def __getattr__(self, item: str) -> Any:
        """Route all unknown attributes to HtmlView."""
        return getattr(self.html_view, item)

    def prepend_tool_bar_widget(self, widget: QtWidgets.QWidget) -> None:
        """
        Insert a widget at the start of the toolbar.

        :param widget: Widget to insert
        """
        self.tool_bar.insertWidget(self.tool_bar.actions()[0], widget)

    def append_tool_bar_widget(self, widget: QtWidgets) -> None:
        """
        Add a widget at the end of the toolbar.

        :param widget: Widget to append
        """
        self.tool_bar.addWidget(widget)


class LogWidget(QtWidgets.QWidget):
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

        self.log_view = LogView()
        self.log_view.prepend_tool_bar_widget(self.log_level_box)
        self.log_view.append_tool_bar_widget(self.clear_button)

        # Layout
        self.tabs = QtWidgets.QTabWidget()
        self.tabs.addTab(self.log_view, self.icon(), self.label())

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
