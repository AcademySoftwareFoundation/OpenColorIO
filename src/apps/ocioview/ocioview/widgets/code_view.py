# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from pygments.formatters import HtmlFormatter
from PySide6 import QtCore, QtGui, QtWidgets

from ..logging import LogRouter
from ..utils import get_glyph_icon
from .text_edit import HtmlView


class CodeView(QtWidgets.QWidget):
    """
    Widget for viewing OCIO and application logs.
    """

    @classmethod
    def label(cls) -> str:
        return "Code"

    @classmethod
    def icon(cls) -> QtGui.QIcon:
        return get_glyph_icon("mdi6.code-json")

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Widgets
        palette = self.palette()
        source_font = QtGui.QFont("Courier")
        source_font.setPointSize(10)

        html_css = HtmlFormatter(style="material").get_style_defs()
        # Update line number colors to match palette
        html_css = html_css.replace("#263238", palette.color(palette.ColorRole.Base).name())
        html_css = html_css.replace(
            "#37474F", palette.color(palette.ColorRole.Text).darker(150).name()
        )

        self.config_view = HtmlView()
        self.config_view.setFont(source_font)
        self.config_view.document().setDefaultStyleSheet(html_css)

        self.ctf_view = HtmlView()
        self.ctf_view.setFont(source_font)
        self.ctf_view.document().setDefaultStyleSheet(html_css)

        self.shader_view = HtmlView()
        self.shader_view.setFont(source_font)
        self.shader_view.document().setDefaultStyleSheet(html_css)

        # Layout
        config_layout = QtWidgets.QVBoxLayout()
        config_layout.addWidget(self.config_view)
        config_frame = QtWidgets.QFrame()
        config_frame.setLayout(config_layout)

        ctf_layout = QtWidgets.QVBoxLayout()
        ctf_layout.addWidget(self.ctf_view)
        ctf_frame = QtWidgets.QFrame()
        ctf_frame.setLayout(ctf_layout)

        shader_layout = QtWidgets.QVBoxLayout()
        shader_layout.addWidget(self.shader_view)
        shader_frame = QtWidgets.QFrame()
        shader_frame.setLayout(shader_layout)

        self.tabs = QtWidgets.QTabWidget()
        self.tabs.addTab(config_frame, get_glyph_icon("mdi6.code-json"), "Config")
        self.tabs.addTab(ctf_frame, get_glyph_icon("mdi6.code-tags"), "Processor (CTF)")
        self.tabs.addTab(
            shader_frame, get_glyph_icon("mdi6.dots-grid"), "Processor (GLSL)"
        )

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.tabs)
        self.setLayout(layout)

        # Initialize
        log_router = LogRouter.get_instance()
        log_router.config_logged.connect(self._on_config_logged)
        log_router.ctf_logged.connect(self._on_ctf_logged)
        log_router.shader_logged.connect(self._on_shader_logged)

    def reset(self) -> None:
        """Clear log history."""
        self.config_view.reset()
        self.shader_view.reset()
        self.ctf_view.reset()

    @QtCore.Slot(str)
    def _on_config_logged(self, record: str) -> None:
        """
        Update config view to show the current OCIO config's YAML
        source.
        """
        self.config_view.setHtml(record)

    @QtCore.Slot(str)
    def _on_ctf_logged(self, record: str) -> None:
        """
        Update CTF view with a lossless XML representation of an
        OCIO processor.
        """
        self.ctf_view.setHtml(record)

    @QtCore.Slot(str)
    def _on_shader_logged(self, record: str) -> None:
        """
        Update shader view with fragment shader source created
        from an OCIO GPU processor.
        """
        self.shader_view.setHtml(record)
