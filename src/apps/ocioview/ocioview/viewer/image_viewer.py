# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

from contextlib import contextmanager
from pathlib import Path
from typing import Generator, Optional, Union

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
from ..items.display_model import DisplayModel
from ..items.view_model import ViewModel
from ..mode import OCIOViewMode
from ..processor_context import ProcessorContext
from ..ref_space_manager import ReferenceSpaceManager
from ..signal_router import SignalRouter
from ..transform_manager import TransformManager
from ..utils import float_to_uint8, get_glyph_icon, SignalsBlocked
from ..widgets import ComboBox, CallbackComboBox, ColorSpaceComboBox
from .image_plane import ImagePlane


class ViewerChannels(object):
    """
    Enum to describe all the toggleable channel view options in
    ``ImagePlane``.
    """

    R, G, B, A, ALL = list(range(5))


class ImageViewer(QtWidgets.QWidget):
    """
    Image viewer widget, which can display an image with internal
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

    ROLE_SLOT = QtCore.Qt.UserRole + 1
    ROLE_ITEM_TYPE = QtCore.Qt.UserRole + 2
    ROLE_ITEM_NAME = QtCore.Qt.UserRole + 3

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
        self._tf_subscription_slot = -1
        self._tf_fwd = None
        self._tf_inv = None

        # Widgets
        # ---------------------------------------------------------------------

        # Viewport
        self.image_plane = ImagePlane(self)
        self.image_plane.setSizePolicy(
            QtWidgets.QSizePolicy(
                QtWidgets.QSizePolicy.Expanding,
                QtWidgets.QSizePolicy.Expanding,
            )
        )

        # Input color space
        self.input_color_space_label = get_glyph_icon(
            "mdi6.import", as_widget=True
        )
        self.input_color_space_label.setToolTip("Input color space")
        self.input_color_space_box = ColorSpaceComboBox(include_roles=True)
        self.input_color_space_box.setToolTip(
            self.input_color_space_label.toolTip()
        )

        # Edit mode
        self.tf_label = get_glyph_icon("mdi6.export", as_widget=True)
        self.tf_box = ComboBox()
        self.tf_box.setToolTip("Output transform")

        self._tf_direction_forward_icon = get_glyph_icon("mdi6.layers-plus")
        self._tf_direction_inverse_icon = get_glyph_icon("mdi6.layers-minus")

        self.tf_direction_button = QtWidgets.QPushButton()
        self.tf_direction_button.setCheckable(True)
        self.tf_direction_button.setChecked(False)
        self.tf_direction_button.setIcon(self._tf_direction_forward_icon)
        self.tf_direction_button.setToolTip("Transform direction: Forward")

        self.output_tf_direction_label = QtWidgets.QLabel("+")

        # Preview mode
        self.display_view_label = get_glyph_icon(
            ViewModel.__icon_glyph__, as_widget=True
        )
        self.display_view_label.setToolTip(
            f"{DisplayModel.item_type_label()} / {ViewModel.item_type_label()}"
        )

        self.display_box = CallbackComboBox(
            self._get_displays,
            get_default_item=self._get_default_display,
        )
        self.display_box.setSizeAdjustPolicy(
            QtWidgets.QComboBox.AdjustToContents
        )
        self.display_box.setToolTip(DisplayModel.item_type_label())
        self.display_box.currentIndexChanged[int].connect(
            self._on_display_changed
        )

        self.view_box = CallbackComboBox(
            self._get_views,
            get_default_item=self._get_default_view,
        )
        self.view_box.setSizeAdjustPolicy(QtWidgets.QComboBox.AdjustToContents)
        self.view_box.setToolTip(ViewModel.item_type_label())
        self.view_box.currentIndexChanged[int].connect(self._on_view_changed)

        # Image adjustments
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

        # Info and inspect labels
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
        # ---------------------------------------------------------------------

        # Info and inspect labels
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

        # Edit mode
        self.tf_layout = QtWidgets.QHBoxLayout()
        self.tf_layout.setContentsMargins(0, 0, 0, 0)
        self.tf_layout.setSpacing(0)
        self.tf_layout.addWidget(self.tf_box)
        self.tf_layout.setStretch(0, 1)
        self.tf_layout.addWidget(self.tf_direction_button)

        self.tf_layout_outer = QtWidgets.QHBoxLayout()
        self.tf_layout_outer.setContentsMargins(0, 0, 0, 0)
        self.tf_layout_outer.addWidget(self.tf_label)
        self.tf_layout_outer.setStretch(0, 0)
        self.tf_layout_outer.addLayout(self.tf_layout)
        self.tf_layout_outer.setStretch(1, 1)

        self.tf_frame = QtWidgets.QFrame()
        self.tf_frame.setLayout(self.tf_layout_outer)

        self.inspect_layout.addWidget(
            self.output_tf_direction_label, 1, 5, QtCore.Qt.AlignRight
        )

        # Preview mode
        self.display_view_layout = QtWidgets.QHBoxLayout()
        self.display_view_layout.setContentsMargins(0, 0, 0, 0)
        self.display_view_layout.setSpacing(0)
        self.display_view_layout.addWidget(self.display_box)
        self.display_view_layout.addWidget(self.view_box)

        self.display_view_layout_outer = QtWidgets.QHBoxLayout()
        self.display_view_layout_outer.setContentsMargins(0, 0, 0, 0)
        self.display_view_layout_outer.addWidget(self.display_view_label)
        self.display_view_layout_outer.setStretch(0, 0)
        self.display_view_layout_outer.addLayout(self.display_view_layout)
        self.display_view_layout_outer.setStretch(1, 1)

        self.display_view_frame = QtWidgets.QFrame()
        self.display_view_frame.setLayout(self.display_view_layout_outer)

        # Mode switch stack
        self.mode_stack = QtWidgets.QStackedWidget()
        self.mode_stack.addWidget(self.tf_frame)  # Edit mode
        self.mode_stack.addWidget(self.display_view_frame)  # Preview mode

        # Input/output
        self.io_layout = QtWidgets.QHBoxLayout()
        self.io_layout.addWidget(self.input_color_space_label)
        self.io_layout.addWidget(self.input_color_space_box)
        self.io_layout.setStretch(1, 1)
        self.io_layout.addWidget(self.mode_stack)
        self.io_layout.setStretch(2, 1)

        # Image adjustments
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

        # Viewport
        self.image_plane_layout = QtWidgets.QVBoxLayout()
        self.image_plane_layout.setSpacing(0)
        self.image_plane_layout.addWidget(self.info_bar)
        self.image_plane_layout.addWidget(self.image_plane)
        self.image_plane_layout.addWidget(self.inspect_bar)

        # Main layout
        layout = QtWidgets.QVBoxLayout()
        layout.addLayout(self.io_layout)
        layout.addLayout(self.adjust_layout)
        layout.addLayout(self.image_plane_layout)
        layout.setStretch(2, 1)
        self.setLayout(layout)

        # Connect signals/slots
        # ---------------------------------------------------------------------

        self.image_plane.image_loaded.connect(self._on_image_loaded)
        self.image_plane.scale_changed.connect(self._on_scale_changed)
        self.image_plane.sample_changed.connect(self._on_sample_changed)
        self.input_color_space_box.color_space_changed.connect(
            self._on_input_color_space_changed
        )
        self.exposure_box.valueChanged.connect(self._on_exposure_changed)
        self.gamma_box.valueChanged.connect(self._on_gamma_changed)
        self.sample_precision_box.valueChanged.connect(
            self._on_sample_precision_changed
        )

        signal_router = SignalRouter.get_instance()
        signal_router.mode_changed.connect(self._on_mode_changed)

        # Edit mode
        self.image_plane.tf_subscription_requested.connect(
            self._on_tf_subscription_requested
        )
        self.tf_box.currentIndexChanged[int].connect(
            self._on_transform_changed
        )
        self.tf_direction_button.clicked[bool].connect(
            self._on_inverse_check_clicked
        )

        # Initialize
        # ---------------------------------------------------------------------

        self._on_sample_precision_changed(self.sample_precision_box.value())
        self._on_sample_changed(-1, -1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

        # Edit mode
        TransformManager.subscribe_to_transform_menu(
            self._on_transform_menu_changed
        )
        TransformManager.subscribe_to_transform_subscription_init(
            self._on_transform_subscription_init
        )

        # Initialize viewport
        self.update(force=True)

    def update(self, force: bool = False, update_items: bool = True) -> None:
        """
        Make this image viewer the current OpenGL rendering context and
        ask it to redraw.

        :param force: Whether to force the image to redraw, regardless
            of OCIO processor changes.
        :param update_items: Whether to update dynamic OCIO items that
            affect image processing.
        """
        mode = OCIOViewMode.current_mode()
        if mode == OCIOViewMode.Preview and update_items:
            self.display_box.update_items()
            self.view_box.update_items()

        self.image_plane.update_ocio_proc(
            proc_context=self._make_processor_context(),
            transform=self._make_transform(),
            force_update=force,
        )

        super().update()

        # Broadcast this viewer's image data for other app components
        self.image_plane.broadcast_image()

    def reset(self) -> None:
        """Reset viewer parameters without unloading the current image."""
        self.image_plane.reset_ocio_proc(update=False)

        # Update widgets to match image plane
        with self._ocio_signals_blocked():
            self.set_transform_direction(ocio.TRANSFORM_DIR_FORWARD)
            self.set_exposure(self.image_plane.exposure())
            self.set_gamma(self.image_plane.gamma())
            self.display_box.reset()
            self.view_box.reset()

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
        return self.input_color_space_box.color_space_name()

    def set_input_color_space(self, color_space: str) -> None:
        """
        Override current input color space. This controls how an input
        image should be interpreted by OCIO. Each loaded image utilizes
        OCIO config file rules to determine this automatically, so this
        override only guarantees persistence for the current image.

        :param color_space: OCIO color space name
        """
        self.input_color_space_box.set_color_space(color_space)

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

    def display(self) -> str:
        """Get current OCIO display."""
        return self.display_box.currentText()

    def view(self) -> str:
        """Get current OCIO view."""
        return self.view_box.currentText()

    def transform(self) -> Optional[ocio.Transform]:
        """Get current OCIO transform."""
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
            or (
                transform_fwd is None
                and tf_direction == ocio.TRANSFORM_DIR_FORWARD
            )
            or (
                transform_inv is None
                and tf_direction == ocio.TRANSFORM_DIR_INVERSE
            )
        ):
            return

        self._tf_fwd = transform_fwd
        self._tf_inv = transform_inv

        self.update()

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

    def transform_item_type(self) -> type | None:
        """Get transform source config item type."""
        return self.tf_box.currentData(role=self.ROLE_ITEM_TYPE)

    def transform_item_name(self) -> str | None:
        """Get transform source config item name."""
        return self.tf_box.currentData(role=self.ROLE_ITEM_NAME)

    def transform_direction(self) -> ocio.TransformDirection:
        """Get transform direction being viewed."""
        return (
            ocio.TRANSFORM_DIR_INVERSE
            if self.tf_direction_button.isChecked()
            else ocio.TRANSFORM_DIR_FORWARD
        )

    def set_transform_direction(
        self, direction: ocio.TransformDirection
    ) -> None:
        """
        :param direction: Set the transform direction to be viewed
        """
        self.tf_direction_button.setChecked(
            direction == ocio.TRANSFORM_DIR_INVERSE
        )

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
        self.display_box.blockSignals(True)
        self.view_box.blockSignals(True)

        yield

        self.input_color_space_box.blockSignals(False)
        self.exposure_box.blockSignals(False)
        self.gamma_box.blockSignals(False)
        self.display_box.blockSignals(False)
        self.view_box.blockSignals(False)

    def _make_processor_context(self) -> ProcessorContext:
        """Create processor context from available data."""
        mode = OCIOViewMode.current_mode()
        if mode == OCIOViewMode.Preview:
            return ProcessorContext(
                self.input_color_space(),
                ViewModel.__item_type__,
                self.view(),
                ocio.TRANSFORM_DIR_FORWARD,
            )
        else:  # Edit
            return ProcessorContext(
                self.input_color_space(),
                self.transform_item_type(),
                self.transform_item_name(),
                self.transform_direction(),
            )

    def _make_transform(self) -> Union[ocio.Transform, None]:
        """Create viewer transform."""
        transform = None
        mode = OCIOViewMode.current_mode()

        if mode == OCIOViewMode.Preview:
            display = self.display()
            view = self.view()

            if display and view:
                # Image plane expects all transforms to be relative to the current
                # config's scene reference space.
                transform = ocio.DisplayViewTransform(
                    src=ReferenceSpaceManager.scene_reference_space().getName(),
                    display=display,
                    view=view,
                    direction=ocio.TRANSFORM_DIR_FORWARD,
                )

        else:  # Edit
            if self._tf_fwd is not None and self._tf_inv is not None:
                if self.transform_direction() == ocio.TRANSFORM_DIR_INVERSE:
                    return self._tf_inv
                else:
                    return self._tf_fwd
            else:
                # Return no-op transform. Returning None instead results in the image
                # plane processor being unchanged, which is problematic when switching
                # application modes, since it will retain the previous display/view
                # transform.
                return ocio.ExponentTransform()

        return transform

    def _get_default_color_space(self) -> str:
        """Get reasonable default color space."""
        all_color_spaces = ConfigCache.get_color_space_names(
            ocio.SEARCH_REFERENCE_SPACE_SCENE
        )
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

    def _get_displays(self) -> list[str]:
        """Get all active OCIO displays."""
        config = ocio.GetCurrentConfig()
        return list(config.getDisplays())

    def _get_default_display(self) -> str:
        """Get default OCIO display."""
        config = ocio.GetCurrentConfig()
        return config.getDefaultDisplay()

    def _get_views(self) -> list[str]:
        """
        Get all active OCIO views, given the current input color space.
        """
        config = ocio.GetCurrentConfig()
        input_color_space = self.input_color_space()
        if input_color_space:
            return config.getViews(self.display(), input_color_space)
        else:
            return config.getViews(self.display())

    def _get_default_view(self) -> str:
        """
        Get default OCIO view, given the current display and input
        color space.
        """
        config = ocio.GetCurrentConfig()
        input_color_space = None
        if input_color_space:
            return config.getDefaultView(self.display(), input_color_space)
        else:
            return config.getDefaultView(self.display())

    def _on_mode_changed(self) -> None:
        """Called when the application mode changes."""
        mode = OCIOViewMode.current_mode()

        if mode == OCIOViewMode.Preview:
            self.mode_stack.setCurrentWidget(self.display_view_frame)
        else:  # Edit
            self.mode_stack.setCurrentWidget(self.tf_frame)

        self.output_tf_direction_label.setVisible(mode == OCIOViewMode.Edit)
        self.update(force=True)

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

    def _on_input_color_space_changed(self) -> None:
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
    def _on_display_changed(self, index: int) -> None:
        """Called when the display changes."""
        with SignalsBlocked(self.view_box):
            self.view_box.update_items()
        self._on_view_changed(0)

    @QtCore.Slot(int)
    def _on_view_changed(self, index: int) -> None:
        """Called when the view changes."""
        self.update(update_items=False)

    @QtCore.Slot(int)
    def _on_sample_precision_changed(self, value: float) -> None:
        self._sample_format = f"{{v:.{value}f}}"

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
            current_slot = self.tf_box.currentData(role=self.ROLE_SLOT)

        with SignalsBlocked(self.tf_box):
            self.tf_box.clear()

            # The first item is always no transform
            self.tf_box.addItem(self.PASSTHROUGH)
            self.tf_box.setItemData(0, -1, role=self.ROLE_SLOT)

            for i, (
                slot,
                item_label,
                item_type,
                item_name,
                slot_icon,
            ) in enumerate(menu_items):
                index = i + 1
                self.tf_box.addItem(slot_icon, item_label)
                self.tf_box.setItemData(index, slot, role=self.ROLE_SLOT)
                self.tf_box.setItemData(
                    index, item_type, role=self.ROLE_ITEM_TYPE
                )
                self.tf_box.setItemData(
                    index, item_name, role=self.ROLE_ITEM_NAME
                )
                if slot == current_slot:
                    target_index = index  # Offset for "Passthrough" item

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
            index = self.tf_box.findData(slot, role=self.ROLE_SLOT)
            if index != -1:
                self.tf_box.setCurrentIndex(index)

    @QtCore.Slot(int)
    def _on_transform_changed(self, index: int) -> None:
        if index == 0:
            TransformManager.unsubscribe_from_all_transforms(
                self.set_transform
            )
            self.clear_transform()
        else:
            self._tf_subscription_slot = self.tf_box.currentData(
                role=self.ROLE_SLOT
            )
            TransformManager.subscribe_to_transforms_at(
                self._tf_subscription_slot, self.set_transform
            )

    @QtCore.Slot(int)
    def _on_tf_subscription_requested(self, slot: int) -> None:
        # If the requested slot does not have a subscription, "Passthrough" will
        # be selected.
        self.tf_box.setCurrentIndex(
            max(0, self.tf_box.findData(slot, role=self.ROLE_SLOT))
        )

    @QtCore.Slot(bool)
    def _on_inverse_check_clicked(self, checked: bool) -> None:
        self.set_transform(
            self._tf_subscription_slot, self._tf_fwd, self._tf_inv
        )
        if self.tf_direction_button.isChecked():
            self.tf_direction_button.setIcon(self._tf_direction_inverse_icon)
            self.tf_direction_button.setToolTip("Transform direction: Inverse")
            # Use 'minus' character to match the width of "+"
            self.output_tf_direction_label.setText("\u2212")
        else:
            self.tf_direction_button.setIcon(self._tf_direction_forward_icon)
            self.tf_direction_button.setToolTip("Transform direction: Forward")
            self.output_tf_direction_label.setText("+")
