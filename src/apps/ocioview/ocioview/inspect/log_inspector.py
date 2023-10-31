# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from ..log_handlers import set_logging_level
from ..message_router import MessageRouter
from ..utils import get_glyph_icon
from ..widgets import ComboBox, LogView


class LogInspector(QtWidgets.QWidget):
    """
    Widget for inspecting OCIO and application logs.
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

        log_router = MessageRouter.get_instance()
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
        set_logging_level(log_level)

    @QtCore.Slot(str)
    def _on_record_logged(self, record: str) -> None:
        """Append record to general log view."""
        self.log_view.append(record)
