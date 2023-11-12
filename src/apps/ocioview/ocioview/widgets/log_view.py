# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Any, Optional

from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import ICON_SIZE_BUTTON
from ..style import (
    apply_top_tool_bar_style,
    apply_widget_with_top_tool_bar_style,
)
from .text_edit import HtmlView


class LogView(QtWidgets.QFrame):
    """Base widget for a log/code viewer and toolbar."""

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        source_font = QtGui.QFont("Courier")
        source_font.setPointSize(10)

        # Widgets
        self.html_view = HtmlView()
        self.html_view.setFont(source_font)

        tool_bar_stretch = QtWidgets.QFrame()
        tool_bar_stretch.setSizePolicy(
            QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed
        )

        self.tool_bar = QtWidgets.QToolBar()
        self.tool_bar.setContentsMargins(0, 0, 0, 0)
        self.tool_bar.setIconSize(ICON_SIZE_BUTTON)
        self.tool_bar.addWidget(tool_bar_stretch)

        # Layout
        tool_bar_layout = QtWidgets.QVBoxLayout()
        tool_bar_layout.setContentsMargins(0, 0, 0, 0)
        tool_bar_layout.addWidget(self.tool_bar)

        tool_bar_frame = QtWidgets.QFrame()
        tool_bar_frame.setFrameShape(QtWidgets.QFrame.StyledPanel)
        tool_bar_frame.setObjectName("base_log_view__tool_bar_frame")
        apply_top_tool_bar_style(tool_bar_frame)
        tool_bar_frame.setLayout(tool_bar_layout)

        inner_layout = QtWidgets.QVBoxLayout()
        inner_layout.setContentsMargins(0, 0, 0, 0)
        inner_layout.setSpacing(1)
        inner_layout.addWidget(tool_bar_frame)
        inner_layout.addWidget(self.html_view)

        inner_frame = QtWidgets.QFrame()
        inner_frame.setFrameShape(QtWidgets.QFrame.StyledPanel)
        inner_frame.setObjectName("base_log_view__log_inner_frame")
        apply_widget_with_top_tool_bar_style(inner_frame)
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
