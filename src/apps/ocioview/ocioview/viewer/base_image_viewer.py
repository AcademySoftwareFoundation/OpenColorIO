# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

from contextlib import contextmanager
from pathlib import Path
from typing import Generator, Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from ..config_cache import ConfigCache
from ..constants import (
    GRAY_COLOR,
    R_COLOR,
    G_COLOR,
    B_COLOR,
    ICON_SIZE_TAB,
)
from ..processor_context import ProcessorContext
from ..utils import float_to_uint8, get_glyph_icon, SignalsBlocked
from ..widgets import CallbackComboBox
from .image_plane import ImagePlane


class ViewerChannels(object):
    """
    Enum to describe all the toggleable channel view options in
    ``ImagePlane``.
    """

    R, G, B, A, ALL = list(range(5))


class BaseImageViewer(QtWidgets.QWidget):
    """
    Base image viewer widget, which can display an image with internal
    32-bit float precision.
    """

    FMT_GRAY_LABEL = f'<span style="color:{GRAY_COLOR.name()};">{{v}}</span>'
    FMT_R_LABEL = f'<span style="color:{R_COLOR.name()};">{{v}}</span>'
    FMT_G_LABEL = f'<span style="color:{G_COLOR.name()};">{{v}}</span>'
    FMT_B_LABEL = f'<span style="color:{B_COLOR.name()};">{{v}}</span>'
    FMT_SWATCH_CSS = "background-color: rgb({r}, {g}, {b});"
    FMT_IMAGE_SCALE = f'{{s:,d}}{FMT_GRAY_LABEL.format(v="%")}'

    @classmethod
    def viewer_type_icon(cls) -> QtGui.QIcon:
        """Get viewer type icon."""
        return get_glyph_icon("mdi6.image-outline", size=ICON_SIZE_TAB)

    @classmethod
    def viewer_type_label(cls) -> str:
        """Get friendly viewer type name."""
        return "Image Viewer"

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent)

        self._sample_format = ""

        # Widgets
        self.image_plane = ImagePlane(self)
        self.image_plane.setSizePolicy(
            QtWidgets.QSizePolicy(
                QtWidgets.QSizePolicy.Expanding,
                QtWidgets.QSizePolicy.Expanding,
            )
        )

        self.input_color_space_label = get_glyph_icon(
            "mdi6.import", as_widget=True
        )
        self.input_color_space_label.setToolTip("Input color space")
        self.input_color_space_box = CallbackComboBox(
            self._get_color_spaces,
            get_default_item=self._get_default_color_space,
        )
        self.input_color_space_box.setToolTip(
            self.input_color_space_label.toolTip()
        )
        self.input_color_space_box.setSizeAdjustPolicy(
            QtWidgets.QComboBox.AdjustToContents
        )

        self.exposure_label = get_glyph_icon("ph.aperture", as_widget=True)
        self.exposure_label.setToolTip("Exposure")
        self.exposure_box = QtWidgets.QDoubleSpinBox()
        self.exposure_box.setToolTip(self.exposure_label.toolTip())
        self.exposure_box.setRange(-6.0, 6.0)
        self.exposure_box.setValue(self.image_plane.exposure())

        self.gamma_label = get_glyph_icon("mdi6.gamma", as_widget=True)
        self.gamma_label.setToolTip("Gamma")
        self.gamma_box = QtWidgets.QDoubleSpinBox()
        self.gamma_box.setStepType(QtWidgets.QSpinBox.AdaptiveDecimalStepType)
        self.gamma_box.setToolTip(self.gamma_label.toolTip())
        self.gamma_box.setRange(0.01, 4.0)
        self.gamma_box.setValue(self.image_plane.gamma())

        self.sample_precision_label = get_glyph_icon(
            "mdi6.decimal-increase", as_widget=True
        )
        self.sample_precision_label.setToolTip(
            "Sample precision (number of digits after the decimal point)"
        )
        self.sample_precision_box = QtWidgets.QSpinBox()
        self.sample_precision_box.setToolTip(
            self.sample_precision_label.toolTip()
        )
        self.sample_precision_box.setValue(5)

        self.image_name_label = QtWidgets.QLabel()
        self.image_scale_label = QtWidgets.QLabel(
            self.FMT_IMAGE_SCALE.format(s=100)
        )

        self.input_w_label = QtWidgets.QLabel(
            self.FMT_GRAY_LABEL.format(v="W:")
        )
        self.image_w_value_label = QtWidgets.QLabel("0")
        self.input_h_label = QtWidgets.QLabel(
            self.FMT_GRAY_LABEL.format(v="H:")
        )
        self.image_h_value_label = QtWidgets.QLabel("0")
        self.input_x_label = QtWidgets.QLabel(
            self.FMT_GRAY_LABEL.format(v="X:")
        )
        self.image_x_value_label = QtWidgets.QLabel("0")
        self.input_y_label = QtWidgets.QLabel(
            self.FMT_GRAY_LABEL.format(v="Y:")
        )
        self.image_y_value_label = QtWidgets.QLabel("0")

        self.input_sample_label = get_glyph_icon(
            "mdi6.import", color=GRAY_COLOR, as_widget=True
        )
        self.input_r_sample_label = QtWidgets.QLabel()
        self.input_g_sample_label = QtWidgets.QLabel()
        self.input_b_sample_label = QtWidgets.QLabel()
        self.input_sample_swatch = QtWidgets.QLabel()
        self.input_sample_swatch.setFixedSize(20, 20)
        self.input_sample_swatch.setStyleSheet(
            self.FMT_SWATCH_CSS.format(r=0, g=0, b=0)
        )

        self.output_sample_label = get_glyph_icon(
            "mdi6.export", color=GRAY_COLOR, as_widget=True
        )
        self.output_r_sample_label = QtWidgets.QLabel()
        self.output_g_sample_label = QtWidgets.QLabel()
        self.output_b_sample_label = QtWidgets.QLabel()
        self.output_sample_swatch = QtWidgets.QLabel()
        self.output_sample_swatch.setFixedSize(20, 20)
        self.output_sample_swatch.setStyleSheet(
            self.FMT_SWATCH_CSS.format(r=0, g=0, b=0)
        )

        # Layout
        self.info_layout = QtWidgets.QHBoxLayout()
        self.info_layout.setContentsMargins(8, 8, 8, 8)
        self.info_layout.addWidget(self.image_name_label)
        self.info_layout.addStretch()
        self.info_layout.addWidget(self.image_scale_label)

        self.info_bar = QtWidgets.QFrame()
        self.info_bar.setObjectName("base_image_viewer__info_bar")
        self.info_bar.setStyleSheet(
            "QFrame#base_image_viewer__info_bar { background-color: black; }"
        )
        self.info_bar.setLayout(self.info_layout)

        self.inspect_layout = QtWidgets.QGridLayout()
        self.inspect_layout.setContentsMargins(8, 8, 8, 8)

        self.inspect_layout.addWidget(
            self.input_w_label, 0, 0, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.image_w_value_label, 0, 1, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.input_h_label, 0, 2, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.image_h_value_label, 0, 3, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(QtWidgets.QLabel(), 0, 4)
        self.inspect_layout.addWidget(
            self.input_sample_label, 0, 6, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.input_r_sample_label, 0, 7, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.input_g_sample_label, 0, 8, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.input_b_sample_label, 0, 9, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.input_sample_swatch, 0, 10, QtCore.Qt.AlignLeft
        )

        self.inspect_layout.addWidget(
            self.input_x_label, 1, 0, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.image_x_value_label, 1, 1, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.input_y_label, 1, 2, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.image_y_value_label, 1, 3, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(QtWidgets.QLabel(), 1, 4)
        self.inspect_layout.setColumnStretch(4, 1)
        self.inspect_layout.addWidget(
            self.output_sample_label, 1, 6, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.output_r_sample_label, 1, 7, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.output_g_sample_label, 1, 8, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.output_b_sample_label, 1, 9, QtCore.Qt.AlignRight
        )
        self.inspect_layout.addWidget(
            self.output_sample_swatch, 1, 10, QtCore.Qt.AlignLeft
        )

        self.inspect_bar = QtWidgets.QFrame()
        self.inspect_bar.setObjectName("base_image_viewer__status_bar")
        self.inspect_bar.setStyleSheet(
            "QFrame#base_image_viewer__status_bar { background-color: black; }"
        )
        self.inspect_bar.setLayout(self.inspect_layout)

        self.io_layout = QtWidgets.QHBoxLayout()
        self.io_layout.addWidget(self.input_color_space_label)
        self.io_layout.addWidget(self.input_color_space_box)
        self.io_layout.setStretch(1, 1)

        self.adjust_layout = QtWidgets.QHBoxLayout()
        self.adjust_layout.addWidget(self.exposure_label)
        self.adjust_layout.addWidget(self.exposure_box)
        self.adjust_layout.setStretch(1, 2)
        self.adjust_layout.addWidget(self.gamma_label)
        self.adjust_layout.addWidget(self.gamma_box)
        self.adjust_layout.setStretch(3, 2)
        self.adjust_layout.addWidget(self.sample_precision_label)
        self.adjust_layout.addWidget(self.sample_precision_box)
        self.adjust_layout.setStretch(5, 1)

        self.image_plane_layout = QtWidgets.QVBoxLayout()
        self.image_plane_layout.setSpacing(0)
        self.image_plane_layout.addWidget(self.info_bar)
        self.image_plane_layout.addWidget(self.image_plane)
        self.image_plane_layout.addWidget(self.inspect_bar)

        layout = QtWidgets.QVBoxLayout()
        layout.addLayout(self.io_layout)
        layout.addLayout(self.adjust_layout)
        layout.addLayout(self.image_plane_layout)
        self.setLayout(layout)

        # Connect signals/slots
        self.image_plane.image_loaded.connect(self._on_image_loaded)
        self.image_plane.scale_changed.connect(self._on_scale_changed)
        self.image_plane.sample_changed.connect(self._on_sample_changed)
        self.input_color_space_box.currentTextChanged[str].connect(
            self._on_input_color_space_changed
        )
        self.exposure_box.valueChanged.connect(self._on_exposure_changed)
        self.gamma_box.valueChanged.connect(self._on_gamma_changed)
        self.sample_precision_box.valueChanged.connect(
            self._on_sample_precision_changed
        )

        # Initialize
        self._on_sample_precision_changed(self.sample_precision_box.value())
        self._on_sample_changed(-1, -1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

    def update(self, force: bool = False, update_items: bool = True) -> None:
        """
        Make this image viewer the current OpenGL rendering context and
        ask it to redraw.

        :param force: Whether to force the image to redraw, regardless
            of OCIO processor changes.
        :param update_items: Whether to update dynamic OCIO items that
            affect image processing.
        """
        super().update()

        # Broadcast this viewer's image data for other app components
        self.image_plane.broadcast_image()

    def reset(self) -> None:
        """Reset viewer parameters without unloading the current image."""
        self.image_plane.reset_ocio_proc(update=False)

        # Update widgets to match image plane
        with SignalsBlocked(self):
            self.set_exposure(self.image_plane.exposure())
            self.set_gamma(self.image_plane.gamma())

        # Update input color spaces and redraw viewport
        self.update()

    def load_image(self, image_path: Path) -> None:
        """
        Load an image into the viewer.

        If no ``image_path`` is provided, a file dialog will allow the
        user to choose one.

        :param image_path: Absolute path to image file
        """
        self.image_plane.load_image(image_path)

        # Input color space could be changed by file rules on image
        # load. Update the GUI without triggering a re-render.
        with self._ocio_signals_blocked():
            self.set_input_color_space(self.image_plane.input_color_space())

    def input_color_space(self) -> str:
        """Get input color space name."""
        return self.input_color_space_box.currentText()

    def set_input_color_space(self, color_space: str) -> None:
        """
        Override current input color space. This controls how an input
        image should be interpreted by OCIO. Each loaded image utilizes
        OCIO config file rules to determine this automatically, so this
        override only guarantees persistence for the current image.

        :param color_space: OCIO color space name
        """
        self.input_color_space_box.update_items()
        self.input_color_space_box.setCurrentText(color_space)

    def view_channel(self, channel: int) -> None:
        """
        Isolate a specific channel by its index. Specifying an out
        of range index will restore the combined channel view.

        :param channel: ImageViewChannels channel view to toggle.
            ALL always shows all channels.
        """
        self.image_plane.update_ocio_proc(channel=channel)

    def exposure(self) -> float:
        """Get exposure value."""
        return self.exposure_box.value()

    def set_exposure(self, value: float) -> None:
        """
        Update viewer exposure, applied in scene_linear space prior to
        the output transform.

        :param value: Exposure value in stops
        """
        self.exposure_box.setValue(value)

    def gamma(self) -> float:
        """Get gamma value."""
        return self.gamma_box.value()

    def set_gamma(self, value: float) -> None:
        """
        Update viewer gamma, applied after the OCIO output transform.

        :param value: Gamma value used like: pow(rgb, 1/gamma)
        """
        self.gamma_box.setValue(value)

    @contextmanager
    def _ocio_signals_blocked(self) -> Generator:
        """
        This context manager can be used to prevent automatic OCIO
        processor updates while changing interconnected OCIO
        parameters.
        """
        self.input_color_space_box.blockSignals(True)
        self.exposure_box.blockSignals(True)
        self.gamma_box.blockSignals(True)

        yield

        self.input_color_space_box.blockSignals(False)
        self.exposure_box.blockSignals(False)
        self.gamma_box.blockSignals(False)

    def _make_processor_context(self) -> ProcessorContext:
        """Create processor context from available data."""
        return ProcessorContext(self.input_color_space())

    def _get_color_spaces(self) -> list[str]:
        """
        Get all active color spaces defined relative to the config's
        scene reference space.
        """
        return ConfigCache.get_color_space_names(
            ocio.SEARCH_REFERENCE_SPACE_SCENE
        )

    def _get_default_color_space(self) -> str:
        """Get reasonable default color space."""
        all_color_spaces = self._get_color_spaces()
        default_color_space = ConfigCache.get_default_color_space_name()
        if (
            default_color_space is not None
            and default_color_space in all_color_spaces
        ):
            return default_color_space
        elif all_color_spaces:
            return all_color_spaces[0]
        else:
            return ""

    @QtCore.Slot(Path, int, int)
    def _on_image_loaded(
        self, image_path: Path, width: int, height: int
    ) -> None:
        self.image_name_label.setText(
            self.FMT_GRAY_LABEL.format(v=image_path.as_posix())
        )
        self.image_w_value_label.setText("0" if width == -1 else str(width))
        self.image_h_value_label.setText("0" if height == -1 else str(height))

    @QtCore.Slot(float)
    def _on_scale_changed(self, scale: float) -> None:
        self.image_scale_label.setText(
            self.FMT_IMAGE_SCALE.format(s=round(scale * 100))
        )

    @QtCore.Slot(int, int, float, float, float, float, float, float)
    def _on_sample_changed(
        self,
        x: int,
        y: int,
        r_input: float,
        g_input: float,
        b_input: float,
        r_output: float,
        g_output: float,
        b_output: float,
    ) -> None:
        # Sample position
        self.image_x_value_label.setText("0" if x == -1 else str(x))
        self.image_y_value_label.setText("0" if y == -1 else str(y))

        # Input pixel sample
        self.input_r_sample_label.setText(
            self.FMT_R_LABEL.format(v=self._sample_format.format(v=r_input))
        )
        self.input_g_sample_label.setText(
            self.FMT_G_LABEL.format(v=self._sample_format.format(v=g_input))
        )
        self.input_b_sample_label.setText(
            self.FMT_B_LABEL.format(v=self._sample_format.format(v=b_input))
        )
        self.input_sample_swatch.setStyleSheet(
            self.FMT_SWATCH_CSS.format(
                r=float_to_uint8(r_input),
                g=float_to_uint8(g_input),
                b=float_to_uint8(b_input),
            )
        )

        # Output pixel sample
        self.output_r_sample_label.setText(
            self.FMT_R_LABEL.format(v=self._sample_format.format(v=r_output))
        )
        self.output_g_sample_label.setText(
            self.FMT_G_LABEL.format(v=self._sample_format.format(v=g_output))
        )
        self.output_b_sample_label.setText(
            self.FMT_B_LABEL.format(v=self._sample_format.format(v=b_output))
        )
        self.output_sample_swatch.setStyleSheet(
            self.FMT_SWATCH_CSS.format(
                r=float_to_uint8(r_output),
                g=float_to_uint8(g_output),
                b=float_to_uint8(b_output),
            )
        )

    @QtCore.Slot(str)
    def _on_input_color_space_changed(self, input_color_space: str) -> None:
        self.image_plane.update_ocio_proc(
            proc_context=self._make_processor_context()
        )

    @QtCore.Slot(float)
    def _on_exposure_changed(self, value: float) -> None:
        self.image_plane.update_exposure(value)

    @QtCore.Slot(float)
    def _on_gamma_changed(self, value: float) -> None:
        self.image_plane.update_gamma(value)

    @QtCore.Slot(int)
    def _on_sample_precision_changed(self, value: float) -> None:
        self._sample_format = f"{{v:.{value}f}}"
