# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from pygments.formatters import HtmlFormatter
from PySide2 import QtCore, QtGui, QtWidgets

from ..utils import get_glyph_icon
from ..widgets import EnumComboBox
from .log_view import LogView
from .log_router import LogRouter
from .utils import processor_to_shader_html


class CodeWidget(QtWidgets.QWidget):
    """
    Widget for viewing OCIO related code, which updates asynchronously,
    but only when visible, to reduce unnecessary background
    processing.
    """

    @classmethod
    def label(cls) -> str:
        return "Code"

    @classmethod
    def icon(cls) -> QtGui.QIcon:
        return get_glyph_icon("mdi6.code-json")

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Source objects for CTF and shader views
        self._prev_group_tf = None
        self._prev_gpu_proc = None

        # HTML style
        palette = self.palette()

        html_css = HtmlFormatter(style="material").get_style_defs()
        # Update line number colors to match palette
        html_css = html_css.replace("#263238", palette.color(palette.Base).name())
        html_css = html_css.replace(
            "#37474F", palette.color(palette.Text).darker(150).name()
        )

        # Widgets
        self.config_view = LogView()
        self.config_view.document().setDefaultStyleSheet(html_css)

        self.export_button = QtWidgets.QToolButton()
        self.export_button.setIcon(get_glyph_icon("mdi6.file-export-outline"))
        self.export_button.setText("Export CTF")
        self.export_button.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)
        self.export_button.released.connect(self._on_export_button_released)

        self.ctf_view = LogView()
        self.ctf_view.document().setDefaultStyleSheet(html_css)
        self.ctf_view.append_tool_bar_widget(self.export_button)

        self.gpu_language_box = EnumComboBox(ocio.GpuLanguage)
        self.gpu_language_box.setSizeAdjustPolicy(QtWidgets.QComboBox.AdjustToContents)
        self.gpu_language_box.set_member(LogRouter.get_instance().get_gpu_language())
        self.gpu_language_box.currentIndexChanged[int].connect(
            self._on_gpu_language_changed
        )

        self.shader_view = LogView()
        self.shader_view.document().setDefaultStyleSheet(html_css)
        self.shader_view.prepend_tool_bar_widget(self.gpu_language_box)

        # Layout
        self.tabs = QtWidgets.QTabWidget()
        self.tabs.addTab(self.config_view, get_glyph_icon("mdi6.code-json"), "Config")
        self.tabs.addTab(
            self.ctf_view, get_glyph_icon("mdi6.code-tags"), "Processor (CTF)"
        )
        self.tabs.addTab(
            self.shader_view, get_glyph_icon("mdi6.dots-grid"), "Processor (GLSL)"
        )

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.tabs)
        self.setLayout(layout)

        # Initialize
        log_router = LogRouter.get_instance()
        log_router.config_logged.connect(self._on_config_logged)
        log_router.ctf_logged.connect(self._on_ctf_logged)
        log_router.shader_logged.connect(self._on_shader_logged)

        self.tabs.currentChanged.connect(self._on_tab_changed)

    def showEvent(self, event: QtGui.QShowEvent) -> None:
        """Start listening to logs for the current tab, if visible."""
        super().showEvent(event)
        self._on_tab_changed(self.tabs.currentIndex())

    def hideEvent(self, event: QtGui.QHideEvent) -> None:
        """Stop listening to logs for all tabs, if not visible."""
        super().hideEvent(event)
        self._on_tab_changed(-1)

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

    @QtCore.Slot(str, ocio.GroupTransform)
    def _on_ctf_logged(self, record: str, group_tf: ocio.GroupTransform) -> None:
        """
        Update CTF view with a lossless XML representation of an
        OCIO processor.
        """
        self._prev_group_tf = group_tf
        self.ctf_view.setHtml(record)

    @QtCore.Slot(str, ocio.GPUProcessor)
    def _on_shader_logged(self, record: str, gpu_proc: ocio.GPUProcessor) -> None:
        """
        Update shader view with fragment shader source created
        from an OCIO GPU processor.
        """
        self._prev_gpu_proc = gpu_proc
        self.shader_view.setHtml(record)

    @QtCore.Slot(int)
    def _on_gpu_language_changed(self, index: int) -> None:
        """
        Update shader language for the current GPU processor and
        LogRouter, which will provide future GPU processors.
        """
        gpu_language = self.gpu_language_box.currentData()
        LogRouter.get_instance().set_gpu_language(gpu_language)
        if self._prev_gpu_proc is not None:
            shader_html_data = processor_to_shader_html(
                self._prev_gpu_proc, gpu_language
            )
            self._on_shader_logged(shader_html_data, self._prev_gpu_proc)

    def _on_export_button_released(self) -> None:
        """Write the current CTF to disk."""
        if self._prev_group_tf is not None:
            ctf_path_str, file_filter = QtWidgets.QFileDialog.getSaveFileName(
                self, "Export CTF File", filter="CTF (*.ctf)"
            )
            if ctf_path_str:
                config = ocio.GetCurrentConfig()
                self._prev_group_tf.write(
                    "Color Transform Format", ctf_path_str, config
                )

    def _on_tab_changed(self, index: int) -> None:
        """Only update visible tabs."""
        log_router = LogRouter.get_instance()

        if index == -1:
            log_router.set_config_updates_allowed(False)
            log_router.set_ctf_updates_allowed(False)
            log_router.set_shader_updates_allowed(False)
            return

        widget = self.tabs.widget(index)

        if widget == self.config_view:
            log_router.set_config_updates_allowed(True)
            log_router.set_ctf_updates_allowed(False)
            log_router.set_shader_updates_allowed(False)
        elif widget == self.ctf_view:
            log_router.set_config_updates_allowed(False)
            log_router.set_ctf_updates_allowed(True)
            log_router.set_shader_updates_allowed(False)
        elif widget == self.shader_view:
            log_router.set_config_updates_allowed(False)
            log_router.set_ctf_updates_allowed(False)
            log_router.set_shader_updates_allowed(True)
