# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from contextlib import contextmanager
from pathlib import Path
from typing import Generator, Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from ..transform_manager import TransformManager
from ..config_cache import ConfigCache
from ..constants import GRAY_COLOR, R_COLOR, G_COLOR, B_COLOR
from ..utils import get_glyph_icon, SignalsBlocked
from ..widgets import ComboBox, CallbackComboBox
from .image_plane import ImagePlane


class ViewerChannels(object):
    """
    Enum to describe all the toggleable channel view options in
    ``ImagePlane``.
    """

    R, G, B, A, ALL = list(range(5))


class ImageViewer(QtWidgets.QWidget):
    """
    Main image viewer widget, which can display an image with internal
    32-bit float precision.
    """

    FMT_GRAY_LABEL = f'<span style="color:{GRAY_COLOR.name()};">{{v}}</span>'
    FMT_R_LABEL = f'<span style="color:{R_COLOR.name()};">{{v}}</span>'
    FMT_G_LABEL = f'<span style="color:{G_COLOR.name()};">{{v}}</span>'
    FMT_B_LABEL = f'<span style="color:{B_COLOR.name()};">{{v}}</span>'
    FMT_SWATCH_CSS = "background-color: rgb({r}, {g}, {b});"
    FMT_IMAGE_SCALE = f'{{s:,d}}{FMT_GRAY_LABEL.format(v="%")}'

    PASSTHROUGH = "passthrough"
    PASSTHROUGH_LABEL = FMT_GRAY_LABEL.format(v=f"{PASSTHROUGH}:")

    WIDGET_HEIGHT_IO = 32

    @classmethod
    def viewer_type_icon(cls) -> QtGui.QIcon:
        """
        :return: Viewer type icon
        """
        return get_glyph_icon("mdi6.image-outline")

    @classmethod
    def viewer_type_label(cls) -> str:
        """
        :return: Friendly viewer type name
        """
        return "Image Viewer"

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent)

        self._tf_subscription_slot = -1
        self._tf_fwd = None
        self._tf_inv = None
        self._sample_format = ""

        # Widgets
        self.image_plane = ImagePlane(self)
        self.image_plane.setSizePolicy(
            QtWidgets.QSizePolicy(
                QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding
            )
        )

        self.input_color_space_label = get_glyph_icon("mdi6.import", as_widget=True)
        self.input_color_space_label.setToolTip("Input color space")
        self.input_color_space_box = CallbackComboBox(
            lambda: ConfigCache.get_color_space_names(ocio.SEARCH_REFERENCE_SPACE_SCENE)
        )
        self.input_color_space_box.setFixedHeight(self.WIDGET_HEIGHT_IO)
        self.input_color_space_box.setToolTip(self.input_color_space_label.toolTip())

        self.tf_label = get_glyph_icon("mdi6.export", as_widget=True)
        self.tf_box = ComboBox()
        self.tf_box.setFixedHeight(self.WIDGET_HEIGHT_IO)
        self.tf_box.setToolTip("Output transform")

        self._tf_direction_forward_icon = get_glyph_icon("mdi6.layers-plus")
        self._tf_direction_inverse_icon = get_glyph_icon("mdi6.layers-minus")

        self.tf_direction_button = QtWidgets.QPushButton()
        self.tf_direction_button.setFixedHeight(self.WIDGET_HEIGHT_IO)
        self.tf_direction_button.setCheckable(True)
        self.tf_direction_button.setChecked(False)
        self.tf_direction_button.setIcon(self._tf_direction_forward_icon)
        self.tf_direction_button.setToolTip("Transform direction: Forward")

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
        self.sample_precision_box.setToolTip(self.sample_precision_label.toolTip())
        self.sample_precision_box.setValue(5)

        self.image_name_label = QtWidgets.QLabel()
        self.image_scale_label = QtWidgets.QLabel(self.FMT_IMAGE_SCALE.format(s=100))

        self.input_w_label = QtWidgets.QLabel(self.FMT_GRAY_LABEL.format(v="W:"))
        self.image_w_value_label = QtWidgets.QLabel("0")
        self.input_h_label = QtWidgets.QLabel(self.FMT_GRAY_LABEL.format(v="H:"))
        self.image_h_value_label = QtWidgets.QLabel("0")
        self.input_x_label = QtWidgets.QLabel(self.FMT_GRAY_LABEL.format(v="X:"))
        self.image_x_value_label = QtWidgets.QLabel("0")
        self.input_y_label = QtWidgets.QLabel(self.FMT_GRAY_LABEL.format(v="Y:"))
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

        self.output_tf_direction_label = QtWidgets.QLabel("+")
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
        info_layout = QtWidgets.QHBoxLayout()
        info_layout.setContentsMargins(8, 8, 8, 8)
        info_layout.addWidget(self.image_name_label)
        info_layout.addStretch()
        info_layout.addWidget(self.image_scale_label)

        self.info_bar = QtWidgets.QFrame()
        self.info_bar.setObjectName("image_viewer__info_bar")
        self.info_bar.setStyleSheet(
            "QFrame#image_viewer__info_bar { background-color: black; }"
        )
        self.info_bar.setLayout(info_layout)

        inspect_layout = QtWidgets.QGridLayout()
        inspect_layout.setContentsMargins(8, 8, 8, 8)

        inspect_layout.addWidget(self.input_w_label, 0, 0, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.image_w_value_label, 0, 1, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.input_h_label, 0, 2, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.image_h_value_label, 0, 3, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(QtWidgets.QLabel(), 0, 4)
        inspect_layout.addWidget(self.input_sample_label, 0, 6, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.input_r_sample_label, 0, 7, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.input_g_sample_label, 0, 8, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.input_b_sample_label, 0, 9, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.input_sample_swatch, 0, 10, QtCore.Qt.AlignLeft)

        inspect_layout.addWidget(self.input_x_label, 1, 0, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.image_x_value_label, 1, 1, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.input_y_label, 1, 2, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.image_y_value_label, 1, 3, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(QtWidgets.QLabel(), 1, 4)
        inspect_layout.setColumnStretch(4, 1)
        inspect_layout.addWidget(
            self.output_tf_direction_label, 1, 5, QtCore.Qt.AlignRight
        )
        inspect_layout.addWidget(self.output_sample_label, 1, 6, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.output_r_sample_label, 1, 7, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.output_g_sample_label, 1, 8, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.output_b_sample_label, 1, 9, QtCore.Qt.AlignRight)
        inspect_layout.addWidget(self.output_sample_swatch, 1, 10, QtCore.Qt.AlignLeft)

        self.inspect_bar = QtWidgets.QFrame()
        self.inspect_bar.setObjectName("image_viewer__status_bar")
        self.inspect_bar.setStyleSheet(
            "QFrame#image_viewer__status_bar { background-color: black; }"
        )
        self.inspect_bar.setLayout(inspect_layout)

        tf_layout = QtWidgets.QHBoxLayout()
        tf_layout.setContentsMargins(0, 0, 0, 0)
        tf_layout.setSpacing(0)
        tf_layout.addWidget(self.tf_box)
        tf_layout.setStretch(0, 1)
        tf_layout.addWidget(self.tf_direction_button)

        io_layout = QtWidgets.QHBoxLayout()
        io_layout.addWidget(self.input_color_space_label)
        io_layout.addWidget(self.input_color_space_box)
        io_layout.setStretch(1, 1)
        io_layout.addWidget(self.tf_label)
        io_layout.addLayout(tf_layout)
        io_layout.setStretch(3, 1)

        adjust_layout = QtWidgets.QHBoxLayout()
        adjust_layout.addWidget(self.exposure_label)
        adjust_layout.addWidget(self.exposure_box)
        adjust_layout.setStretch(1, 2)
        adjust_layout.addWidget(self.gamma_label)
        adjust_layout.addWidget(self.gamma_box)
        adjust_layout.setStretch(3, 2)
        adjust_layout.addWidget(self.sample_precision_label)
        adjust_layout.addWidget(self.sample_precision_box)
        adjust_layout.setStretch(5, 1)

        image_plane_layout = QtWidgets.QVBoxLayout()
        image_plane_layout.setSpacing(0)
        image_plane_layout.addWidget(self.info_bar)
        image_plane_layout.addWidget(self.image_plane)
        image_plane_layout.addWidget(self.inspect_bar)

        layout = QtWidgets.QVBoxLayout()
        layout.addLayout(io_layout)
        layout.addLayout(adjust_layout)
        layout.addLayout(image_plane_layout)
        self.setLayout(layout)

        # Connect signals/slots
        self.image_plane.image_loaded.connect(self._on_image_loaded)
        self.image_plane.scale_changed.connect(self._on_scale_changed)
        self.image_plane.sample_changed.connect(self._on_sample_changed)
        self.image_plane.tf_subscription_requested.connect(
            self._on_tf_subscription_requested
        )
        self.input_color_space_box.currentTextChanged[str].connect(
            self._on_input_color_space_changed
        )
        self.tf_box.currentIndexChanged[int].connect(self._on_transform_changed)
        self.tf_direction_button.clicked[bool].connect(self._on_inverse_check_clicked)
        self.exposure_box.valueChanged.connect(self._on_exposure_changed)
        self.gamma_box.valueChanged.connect(self._on_gamma_changed)
        self.sample_precision_box.valueChanged.connect(
            self._on_sample_precision_changed
        )

        # Initialize
        TransformManager.subscribe_to_transform_menu(self._on_transform_menu_changed)
        TransformManager.subscribe_to_transform_subscription_init(
            self._on_transform_subscription_init
        )
        self.update()
        self._on_sample_precision_changed(self.sample_precision_box.value())
        self._on_sample_changed(-1, -1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

    def update(self, force: bool = False) -> None:
        """
        Make this image viewer the current OpenGL rendering context and
        ask it to redraw.

        :param force: Whether to force the image to redraw, regardless
            of OCIO processor changes.
        """
        self._update_input_color_spaces(update=False)

        self.image_plane.makeCurrent()
        self.image_plane.update_ocio_proc(
            input_color_space=self.input_color_space(), force_update=force
        )

        super().update()

    def reset(self) -> None:
        """Reset viewer parameters without unloading the current image."""
        self.image_plane.reset_ocio_proc(update=False)

        # Update widgets to match image plane
        with SignalsBlocked(self):
            self.set_transform_direction(False)
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

    def view_channel(self, channel: int) -> None:
        """
        Isolate a specific channel by its index. Specifying an out
        of range index will restore the combined channel view.

        :param channel: ImageViewChannels channel view to toggle.
            ALL always shows all channels.
        """
        self.image_plane.update_ocio_proc(channel=channel)

    def input_color_space(self) -> str:
        """
        :return: Input color space name
        """
        return self.input_color_space_box.currentText()

    def set_input_color_space(self, color_space: str) -> None:
        """
        Override current input color space. This controls how an input
        image should be interpreted by OCIO. Each loaded image utilizes
        OCIO config file rules to determine this automatically, so this
        override only guarantees persistence for the current image.

        :param color_space: OCIO color space name
        """
        self._update_input_color_spaces(update=False)
        self.input_color_space_box.setCurrentText(color_space)

    def transform(self) -> Optional[ocio.Transform]:
        """
        :return: Current OCIO transform
        """
        return self.image_plane.transform()

    def set_transform(
        self,
        slot: int,
        transform_fwd: Optional[ocio.Transform],
        transform_inv: Optional[ocio.Transform],
    ) -> None:
        """
        Update main OCIO transform for the viewing pipeline, to be
        applied from the current config's scene reference space.

        :param slot: Transform subscription slot
        :param transform_fwd: Forward transform
        :param transform_inv: Inverse transform
        """
        tf_direction = self.transform_direction()

        if (
            slot != self._tf_subscription_slot
            or (transform_fwd is None and tf_direction == ocio.TRANSFORM_DIR_FORWARD)
            or (transform_inv is None and tf_direction == ocio.TRANSFORM_DIR_INVERSE)
        ):
            return

        self._tf_fwd = transform_fwd
        self._tf_inv = transform_inv

        self.image_plane.update_ocio_proc(
            transform=self._tf_inv
            if tf_direction == ocio.TRANSFORM_DIR_INVERSE
            else self._tf_fwd
        )

    def clear_transform(self) -> None:
        """
        Clear current OCIO transform, passing through the input image.
        """
        self._tf_subscription_slot = -1
        self._tf_fwd = None
        self._tf_inv = None

        if self.tf_box.currentIndex() != 0:
            with SignalsBlocked(self.tf_box):
                self.tf_box.setCurrentIndex(0)

        self.image_plane.clear_transform()

    def transform_direction(self) -> ocio.TransformDirection:
        """
        :return: Transform direction being viewed
        """
        return (
            ocio.TRANSFORM_DIR_INVERSE
            if self.tf_direction_button.isChecked()
            else ocio.TRANSFORM_DIR_FORWARD
        )

    def set_transform_direction(self, direction: ocio.TransformDirection) -> None:
        """
        :param direction: Set the transform direction to be viewed
        """
        self.tf_direction_button.setChecked(direction == ocio.TRANSFORM_DIR_INVERSE)

    def exposure(self) -> float:
        """
        :return: Exposure value
        """
        return self.exposure_box.value()

    def set_exposure(self, value: float) -> None:
        """
        Update viewer exposure, applied in scene_linear space prior to
        the output transform.

        :param value: Exposure value in stops
        """
        self.exposure_box.setValue(value)

    def gamma(self) -> float:
        """
        :return: Gamma value
        """
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

    def _update_input_color_spaces(self, update: bool = True) -> None:
        """
        If the current color space is no longer available, reload all
        input color spaces and choose a reasonable default.
        """
        color_space_names = ConfigCache.get_color_space_names(
            ocio.SEARCH_REFERENCE_SPACE_SCENE
        )
        if (
            not self.input_color_space_box.count()
            or self.input_color_space() not in color_space_names
        ):
            default_color_space = ConfigCache.get_default_color_space_name()

            with self._ocio_signals_blocked():
                self.input_color_space_box.clear()
                self.input_color_space_box.addItems(color_space_names)
                self.input_color_space_box.setCurrentText(default_color_space)

        if update:
            self._on_input_color_space_changed(self.input_color_space())

    def _on_transform_menu_changed(
        self, menu_items: list[tuple[int, str, QtGui.QIcon]]
    ) -> None:
        """
        Called to refresh transform menu items, and either reselect the
        existing subscription, or deselect any subscription.
        """
        target_index = -1
        current_slot = -1
        if self.tf_box.count():
            current_slot = self.tf_box.currentData()

        with SignalsBlocked(self.tf_box):
            self.tf_box.clear()

            # The first item is always no transform
            self.tf_box.addItem(self.PASSTHROUGH, userData=-1)

            for i, (slot, item_name, item_type_icon) in enumerate(menu_items):
                self.tf_box.addItem(item_type_icon, item_name, userData=slot)
                if slot == current_slot:
                    target_index = i + 1  # Offset for "Passthrough" item

            # Restore previous item?
            if target_index != -1:
                self.tf_box.setCurrentIndex(target_index)

        # Switch to "Passthrough" if previous slot not found
        if target_index == -1 and self.tf_box.count():
            with SignalsBlocked(self.tf_box):
                self.tf_box.setCurrentIndex(0)

            # Force update transform
            self._on_transform_changed(0)

    def _on_transform_subscription_init(self, slot: int) -> None:
        """
        If this viewer is not subscribed to a specific transform
        subscription slot, subscribe to the first slot to receive a
        transform subscription.

        :param slot: Transform subscription slot
        """
        if self._tf_subscription_slot == -1:
            index = self.tf_box.findData(slot)
            if index != -1:
                self.tf_box.setCurrentIndex(index)

    def _float_to_uint8(self, value: float) -> int:
        """
        :param value: Float value
        :return: 8-bit clamped unsigned integer value
        """
        return max(0, min(255, int(value * 255)))

    @QtCore.Slot(int)
    def _on_transform_changed(self, index: int) -> None:
        if index == 0:
            TransformManager.unsubscribe_from_all_transforms(self.set_transform)
            self.clear_transform()
        else:
            self._tf_subscription_slot = self.tf_box.currentData()
            TransformManager.subscribe_to_transforms_at(
                self._tf_subscription_slot, self.set_transform
            )

    @QtCore.Slot(int)
    def _on_tf_subscription_requested(self, slot: int) -> None:
        # If the requested slot does not have a subscription, "Passthrough" will
        # be selected.
        self.tf_box.setCurrentIndex(max(0, self.tf_box.findData(slot)))

    @QtCore.Slot(bool)
    def _on_inverse_check_clicked(self, checked: bool) -> None:
        self.set_transform(self._tf_subscription_slot, self._tf_fwd, self._tf_inv)
        if self.tf_direction_button.isChecked():
            self.tf_direction_button.setIcon(self._tf_direction_inverse_icon)
            self.tf_direction_button.setToolTip("Transform direction: Inverse")
            # Use 'minus' character to match the width of "+"
            self.output_tf_direction_label.setText("\u2212")
        else:
            self.tf_direction_button.setIcon(self._tf_direction_forward_icon)
            self.tf_direction_button.setToolTip("Transform direction: Forward")
            self.output_tf_direction_label.setText("+")

    @QtCore.Slot(Path, int, int)
    def _on_image_loaded(self, image_path: Path, width: int, height: int) -> None:
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
                r=self._float_to_uint8(r_input),
                g=self._float_to_uint8(g_input),
                b=self._float_to_uint8(b_input),
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
                r=self._float_to_uint8(r_output),
                g=self._float_to_uint8(g_output),
                b=self._float_to_uint8(b_output),
            )
        )

    @QtCore.Slot(str)
    def _on_input_color_space_changed(self, input_color_space: str) -> None:
        self.image_plane.update_ocio_proc(input_color_space=input_color_space)

    @QtCore.Slot(float)
    def _on_exposure_changed(self, value: float) -> None:
        self.image_plane.update_exposure(value)

    @QtCore.Slot(float)
    def _on_gamma_changed(self, value: float) -> None:
        self.image_plane.update_gamma(value)

    @QtCore.Slot(int)
    def _on_sample_precision_changed(self, value: float) -> None:
        self._sample_format = f"{{v:.{value}f}}"
