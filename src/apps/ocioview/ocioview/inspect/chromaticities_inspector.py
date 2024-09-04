# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from itertools import groupby
from typing import Optional

import colour
import pygfx as gfx
import numpy as np
import PyOpenColorIO as ocio
from colour_visuals import (
    VisualChromaticityDiagram,
    VisualGrid,
    VisualRGBColourspace2D,
    VisualRGBColourspace3D,
    VisualRGBScatter3D,
    VisualSpectralLocus2D,
)
from colour import CCS_ILLUMINANTS, RGB_Colourspace, XYZ_to_RGB
from PySide6 import QtCore, QtGui, QtWidgets

from ..config_cache import ConfigCache
from ..constants import ICON_SIZE_TAB
from ..message_router import MessageRouter
from ..processor_context import ProcessorContext
from ..utils import (
    color_space_to_rgb_colourspace,
    get_glyph_icon,
    subsampling_factor,
)
from ..viewer import WgpuCanvasOffScreenViewer


class ChromaticitiesInspector(QtWidgets.QWidget):
    """
    Widget for inspecting chromaticities of the loaded image.

    The image processing from its input to chromaticities display is as follows:

    1.  The current active OCIO processor is applied to the image, converting
        from the *input color space* to the *output transform color space*.
    2.  The image is converted from the *chromaticities color space* to the
        *CIE-XYZ-D65 interchange color space. This space must be defined in the
        config.
    3.  The resulting image is then converted from the *CIE-XYZ-D65 interchange
        color space* to the internal :class:`VisualRGBScatter3D` class instance
        working space.

    The *input and chromaticities color space* 2D and 3D gamuts are automatically
    generated from the config by transforming to the *CIE-XYZ-D65 interchange
    color space* and producing a :class:`colour.RGB_Colourspace` class instance.
    """

    MAXIMUM_IMAGE_SAMPLES_COUNT = 1e6
    """Maximum number of samples from the image to display."""
    COLOR_BACKGROUND = np.array([0.18, 0.18, 0.18])
    """Background color of the *WebGPU* viewer."""
    COLOR_RGB_COLORSPACE_INPUT = np.array([1, 0.5, 0.25])
    """Color associated with the input RGB colorspace."""
    COLOR_RGB_COLORSPACE_CHROMATICITIES = np.array([0.25, 1, 0.5])
    """Color associated with the chromaticities RGB colorspace."""

    @classmethod
    def label(cls) -> str:
        return "Chromaticities"

    @classmethod
    def icon(cls) -> QtGui.QIcon:
        return get_glyph_icon("mdi6.grain", size=ICON_SIZE_TAB)

    def __init__(self, parent: Optional[QtCore.QObject] = None) -> None:
        super().__init__(parent=parent)

        colour.utilities.filter_warnings(*[True] * 4)
        colour.plotting.CONSTANTS_COLOUR_STYLE.font.size = 20

        self._context = None
        self._processor = None
        self._image_array = np.atleast_3d([0, 0, 0]).astype(np.float32)

        # Chromaticity Diagram Working Space
        self._working_whitepoint = CCS_ILLUMINANTS[
            "CIE 1931 2 Degree Standard Observer"
        ]["D65"]
        working_space = RGB_Colourspace(
            "CIE-XYZ-D65",
            colour.XYZ_to_xy(np.identity(3)),
            self._working_whitepoint,
            "D65",
            use_derived_matrix_RGB_to_XYZ=True,
            use_derived_matrix_XYZ_to_RGB=True,
        )
        colour.RGB_COLOURSPACES[working_space.name] = working_space
        self._working_space = working_space.name

        # Widgets
        self._wgpu_viewer = WgpuCanvasOffScreenViewer()
        self._conversion_chain_label = QtWidgets.QLabel()
        self._conversion_chain_label.setStyleSheet(
            ".QLabel { font-size: 10pt;qproperty-alignment: AlignCenter;}"
        )

        self._chromaticities_color_spaces_label = QtWidgets.QLabel(
            "Chromaticities Color Space"
        )
        self._chromaticities_color_spaces_combobox = QtWidgets.QComboBox()
        self._chromaticities_color_spaces_combobox.setToolTip(
            "Chromaticities Color Space"
        )

        self._method_label = get_glyph_icon("mdi.grid", as_widget=True)
        self._method_label.setToolTip("Method")
        self._method_combobox = QtWidgets.QComboBox()
        self._method_combobox.addItems(
            ["CIE 1931", "CIE 1960 UCS", "CIE 1976 UCS"]
        )
        self._method_combobox.setToolTip("Method")

        self._draw_input_color_space_pushbutton = QtWidgets.QPushButton()
        self._draw_input_color_space_pushbutton.setIcon(
            get_glyph_icon("mdi.import")
        )
        self._draw_input_color_space_pushbutton.setCheckable(True)
        self._draw_input_color_space_pushbutton.setToolTip(
            "Draw Input Color Space"
        )

        self._draw_chromaticities_color_space_pushbutton = (
            QtWidgets.QPushButton()
        )
        self._draw_chromaticities_color_space_pushbutton.setIcon(
            get_glyph_icon("mdi.set-none")
        )
        self._draw_chromaticities_color_space_pushbutton.setCheckable(True)
        self._draw_chromaticities_color_space_pushbutton.setToolTip(
            "Draw Chromaticities Color Space"
        )

        self._use_3d_visuals_pushbutton = QtWidgets.QPushButton()
        self._use_3d_visuals_pushbutton.setIcon(
            get_glyph_icon("mdi.cube-outline")
        )
        self._use_3d_visuals_pushbutton.setCheckable(True)
        self._use_3d_visuals_pushbutton.setToolTip("Use 3D Visuals")

        self._use_orthographic_projection_pushbutton = QtWidgets.QPushButton()
        self._use_orthographic_projection_pushbutton.setIcon(
            get_glyph_icon("mdi.camera-control")
        )
        self._use_orthographic_projection_pushbutton.setCheckable(True)
        self._use_orthographic_projection_pushbutton.setToolTip(
            "Orthographic Projection"
        )

        self._reset_camera_pushbutton = QtWidgets.QPushButton()
        self._reset_camera_pushbutton.setIcon(get_glyph_icon("mdi.restart"))
        self._reset_camera_pushbutton.setToolTip("Reset Camera")

        # Layout
        vbox_layout = QtWidgets.QVBoxLayout()
        vbox_layout.addWidget(self._conversion_chain_label)
        spacer = QtWidgets.QSpacerItem(
            20,
            20,
            QtWidgets.QSizePolicy.Expanding,
            QtWidgets.QSizePolicy.Expanding,
        )
        vbox_layout.addItem(spacer)
        self._wgpu_viewer.setLayout(vbox_layout)

        hbox_layout = QtWidgets.QHBoxLayout()
        hbox_layout.addWidget(self._chromaticities_color_spaces_label)
        hbox_layout.addWidget(self._chromaticities_color_spaces_combobox)
        hbox_layout.setStretch(1, 1)

        hbox_layout.addWidget(self._method_label)
        hbox_layout.addWidget(self._method_combobox)

        hbox_layout.addWidget(self._draw_input_color_space_pushbutton)
        hbox_layout.addWidget(self._draw_chromaticities_color_space_pushbutton)
        hbox_layout.addWidget(self._use_3d_visuals_pushbutton)
        hbox_layout.addWidget(self._use_orthographic_projection_pushbutton)
        hbox_layout.addWidget(self._reset_camera_pushbutton)

        vbox_layout = QtWidgets.QVBoxLayout()
        vbox_layout.addLayout(hbox_layout)
        vbox_layout.addWidget(self._wgpu_viewer)
        self.setLayout(vbox_layout)

        msg_router = MessageRouter.get_instance()
        msg_router.config_html_ready.connect(self._on_config_html_ready)
        msg_router.processor_ready.connect(self._on_processor_ready)
        msg_router.image_ready.connect(self._on_image_ready)

        # Visuals
        self._wgpu_viewer.wgpu_scene.add(
            gfx.Background(None, gfx.BackgroundMaterial(self.COLOR_BACKGROUND))
        )

        method = "CIE 1931"
        self._visuals = {
            "grid": VisualGrid(size=4),
            "spectral_locus": VisualSpectralLocus2D(
                method=method,
            ),
            "chromaticity_diagram": VisualChromaticityDiagram(
                method=method,
                opacity=0.25,
            ),
            "rgb_color_space_input_2d": VisualRGBColourspace2D(
                self._working_space,
                colour=self.COLOR_RGB_COLORSPACE_INPUT,
                thickness=2,
            ),
            "rgb_color_space_input_3d": VisualRGBColourspace3D(
                self._working_space,
                wireframe=True,
                segments=24,
            ),
            "rgb_color_space_chromaticities_2d": VisualRGBColourspace2D(
                self._working_space,
                colour=self.COLOR_RGB_COLORSPACE_CHROMATICITIES,
                thickness=2,
            ),
            "rgb_color_space_chromaticities_3d": VisualRGBColourspace3D(
                self._working_space,
                wireframe=True,
                segments=24,
            ),
            "rgb_scatter_3d": VisualRGBScatter3D(
                np.zeros(3), self._working_space, model=method, size=4
            ),
        }

        self._root = gfx.Group()
        for visual in self._visuals.values():
            self._root.add(visual)

        self._setup_widgets()
        self._setup_visuals()
        self._setup_notifications()

    @property
    def wgpu_viewer(self) -> WgpuCanvasOffScreenViewer:
        """
        Getter property for the *WebGPU* viewer.

        :return: *WebGPU* viewer.
        """

        return self._wgpu_viewer

    def reset(self) -> None:
        """Resets the widgets and *Visuals* to their initial state."""

        self._setup_widgets()
        self._setup_visuals()

    def showEvent(self, event: QtGui.QShowEvent) -> None:
        """Start listening for processor updates, if visible."""
        super().showEvent(event)

        msg_router = MessageRouter.get_instance()
        # NOTE: We need to be able to receive notifications about config changes
        # and this is currently the only way to do that without connecting
        # to the `ConfigDock.config_changed` signal.
        msg_router.config_updates_allowed = True
        msg_router.processor_updates_allowed = True
        msg_router.image_updates_allowed = True

    def hideEvent(self, event: QtGui.QHideEvent) -> None:
        """Stop listening for processor updates, if not visible."""
        super().hideEvent(event)

        msg_router = MessageRouter.get_instance()
        msg_router.config_updates_allowed = False
        msg_router.processor_updates_allowed = False
        msg_router.image_updates_allowed = False

    def _set_rgb_color_space_input_visuals_visibility(self) -> None:
        """Set the visibility of the *input color space* related *Visuals*."""

        use_3d_visuals = self._use_3d_visuals_pushbutton.isChecked()
        draw_input_color_space = (
            self._draw_input_color_space_pushbutton.isChecked()
        )

        self._visuals["rgb_color_space_input_2d"].visible = (
            not use_3d_visuals
        ) * draw_input_color_space
        self._visuals["rgb_color_space_input_3d"].visible = (
            use_3d_visuals * draw_input_color_space
        )

        self._wgpu_viewer.render()

    def _set_rgb_color_space_chromaticities_visuals_visibility(self) -> None:
        """Set the visibility of the *chromaticities color space* related *Visuals*."""

        use_3d_visuals = self._use_3d_visuals_pushbutton.isChecked()
        draw_chromaticities_color_space = (
            self._draw_chromaticities_color_space_pushbutton.isChecked()
        )

        self._visuals["rgb_color_space_chromaticities_2d"].visible = (
            not use_3d_visuals
        ) * draw_chromaticities_color_space
        self._visuals["rgb_color_space_chromaticities_3d"].visible = (
            use_3d_visuals * draw_chromaticities_color_space
        )

        self._wgpu_viewer.render()

    def _on_draw_input_color_space_pushbutton_clicked(
        self, state: bool
    ) -> None:
        """
        Slot triggered when the `draw_input_color_space_pushbutton` widget
        is clicked.
        """

        self._set_rgb_color_space_input_visuals_visibility()

    def _on_draw_chromaticities_color_space_pushbutton_clicked(
        self, state: bool
    ) -> None:
        """
        Slot triggered when the `draw_chromaticities_color_space_pushbutton`
        widget is clicked.
        """

        self._set_rgb_color_space_chromaticities_visuals_visibility()

    def _on_use_3d_visuals_pushbutton_clicked(self, state: bool) -> None:
        """
        Slot triggered when the `use_3d_visuals_pushbutton` widget is clicked.
        """

        self._set_rgb_color_space_input_visuals_visibility()
        self._set_rgb_color_space_chromaticities_visuals_visibility()

    def _on_use_orthographic_projection_pushbutton_clicked(
        self, state: bool
    ) -> None:
        """
        Slot triggered when the `use_orthographic_projection_pushbutton` widget
        is clicked.
        """

        self._wgpu_viewer.wgpu_camera.fov = 0 if state else 50

        self._wgpu_viewer.render()

    def _setup_widgets(self) -> None:
        """Setup the widgets initial state."""

        self._draw_input_color_space_pushbutton.setChecked(True)
        self._draw_chromaticities_color_space_pushbutton.setChecked(True)
        self._use_3d_visuals_pushbutton.setChecked(False)
        self._use_orthographic_projection_pushbutton.setChecked(True)

        self._set_rgb_color_space_input_visuals_visibility()
        self._set_rgb_color_space_chromaticities_visuals_visibility()

    def _setup_visuals(self) -> None:
        """Setup the *Visuals* initial state."""

        self._visuals["rgb_color_space_input_2d"].visible = False
        self._visuals["rgb_color_space_input_2d"].local.position = np.array(
            [0, 0, 0.000025]
        )
        self._visuals["rgb_color_space_input_3d"].visible = False
        self._visuals["rgb_color_space_chromaticities_2d"].visible = False
        self._visuals[
            "rgb_color_space_chromaticities_2d"
        ].local.position = np.array([0, 0, 0.00005])
        self._visuals["rgb_color_space_chromaticities_3d"].visible = False
        self._visuals["rgb_scatter_3d"].visible = False

        self._wgpu_viewer.wgpu_scene.add(self._root)

        self._reset_camera()

    def _setup_notifications(self) -> None:
        """Setup the widgets notifications, i.e., signals and slots."""

        self._chromaticities_color_spaces_combobox.textActivated.connect(
            self._update_visuals
        )
        self._method_combobox.textActivated.connect(self._update_visuals)
        self._draw_input_color_space_pushbutton.clicked.connect(
            self._on_draw_input_color_space_pushbutton_clicked
        )
        self._draw_chromaticities_color_space_pushbutton.clicked.connect(
            self._on_draw_chromaticities_color_space_pushbutton_clicked
        )
        self._use_3d_visuals_pushbutton.clicked.connect(
            self._on_use_3d_visuals_pushbutton_clicked
        )
        self._use_orthographic_projection_pushbutton.clicked.connect(
            self._on_use_orthographic_projection_pushbutton_clicked
        )
        self._reset_camera_pushbutton.clicked.connect(self._reset_camera)

    @QtCore.Slot(str)
    def _on_config_html_ready(self, record: str) -> None:
        """Slot triggered when the *Config* html is updated."""

        color_space_names = ConfigCache.get_color_space_names()

        items = [
            self._chromaticities_color_spaces_combobox.itemText(i)
            for i in range(self._chromaticities_color_spaces_combobox.count())
        ]

        if items != color_space_names:
            self._chromaticities_color_spaces_combobox.clear()
            self._chromaticities_color_spaces_combobox.addItems(
                color_space_names
            )

        config = ocio.GetCurrentConfig()
        has_role_interchange_display = config.hasRole(
            ocio.ROLE_INTERCHANGE_DISPLAY
        )
        self._chromaticities_color_spaces_combobox.setEnabled(
            has_role_interchange_display
        )

        self._draw_input_color_space_pushbutton.setEnabled(
            has_role_interchange_display
        )
        self._set_rgb_color_space_input_visuals_visibility()
        self._visuals[
            "rgb_color_space_input_2d"
        ].visible *= has_role_interchange_display
        self._visuals[
            "rgb_color_space_input_3d"
        ].visible *= has_role_interchange_display

        self._draw_chromaticities_color_space_pushbutton.setEnabled(
            has_role_interchange_display
        )
        self._set_rgb_color_space_chromaticities_visuals_visibility()
        self._visuals[
            "rgb_color_space_chromaticities_2d"
        ].visible *= has_role_interchange_display
        self._visuals[
            "rgb_color_space_chromaticities_3d"
        ].visible *= has_role_interchange_display

    @QtCore.Slot(ocio.CPUProcessor)
    def _on_processor_ready(
        self, proc_context: ProcessorContext, cpu_proc: ocio.CPUProcessor
    ) -> None:
        """
        Slot triggered when the *OCIO* processor is ready.

        The processor and context are stored and can then be used to process
        the image when it is itself ready.
        """

        self._context = proc_context
        self._processor = cpu_proc

        self._update_visuals()

    @QtCore.Slot(np.ndarray)
    def _on_image_ready(self, image_array: np.ndarray) -> None:
        """
        Slot triggered when the image is ready.
        """

        sub_sampling_factor = int(
            np.sqrt(
                subsampling_factor(
                    image_array, self.MAXIMUM_IMAGE_SAMPLES_COUNT
                )
            )
        )
        self._image_array = image_array[
            ::sub_sampling_factor, ::sub_sampling_factor
        ]

        self._visuals["rgb_scatter_3d"].visible = True

        self._update_visuals()

    def _update_visuals(self, *args):
        """
        Update the *Visuals* to the desired state.
        """

        conversion_chain = []

        image_array = np.copy(self._image_array)
        # Don't try to process single or zero pixel images
        image_empty = image_array.size <= 3

        # 1. Apply current active processor
        if not image_empty and self._processor is not None:
            if self._context.transform_item_name is not None:
                conversion_chain += [
                    self._context.input_color_space,
                    self._context.transform_item_name,
                ]

            rgb_colourspace = color_space_to_rgb_colourspace(
                self._context.input_color_space
            )

            if rgb_colourspace is not None:
                self._visuals[
                    "rgb_color_space_input_2d"
                ].colourspace = rgb_colourspace
                self._visuals[
                    "rgb_color_space_input_3d"
                ].colourspace = rgb_colourspace
            self._processor.applyRGB(image_array)

        # 2. Convert from chromaticities input space to "CIE-XYZ-D65" interchange
        config = ocio.GetCurrentConfig()
        input_color_space = (
            self._chromaticities_color_spaces_combobox.currentText()
        )
        if (
            config.hasRole(ocio.ROLE_INTERCHANGE_DISPLAY)
            and input_color_space in ConfigCache.get_color_space_names()
        ):
            chromaticities_colorspace = (
                self._chromaticities_color_spaces_combobox.currentText()
            )
            conversion_chain += [
                chromaticities_colorspace,
                ocio.ROLE_INTERCHANGE_DISPLAY.replace(
                    "cie_xyz_d65_interchange", "CIE-XYZ-D65"
                ),
            ]

            rgb_colourspace = color_space_to_rgb_colourspace(
                chromaticities_colorspace
            )

            if rgb_colourspace is not None:
                self._visuals[
                    "rgb_color_space_chromaticities_2d"
                ].colourspace = rgb_colourspace
                self._visuals[
                    "rgb_color_space_chromaticities_3d"
                ].colourspace = rgb_colourspace

            colorspace_transform = ocio.ColorSpaceTransform(
                src=chromaticities_colorspace,
                dst=ocio.ROLE_INTERCHANGE_DISPLAY,
            )
            processor = config.getProcessor(
                colorspace_transform, ocio.TRANSFORM_DIR_FORWARD
            ).getDefaultCPUProcessor()
            processor.applyRGB(image_array)

        # 3. Convert from "CIE-XYZ-D65" to "VisualRGBScatter3D" working space
        conversion_chain += ["CIE-XYZ-D65", self._working_space]

        if not image_empty:
            image_array = XYZ_to_RGB(
                image_array,
                self._working_space,
                illuminant=self._working_whitepoint,
            )

        conversion_chain = [
            color_space for color_space, _group in groupby(conversion_chain)
        ]

        if len(conversion_chain) == 1:
            conversion_chain = []

        self._conversion_chain_label.setText(" → ".join(conversion_chain))

        self._visuals["rgb_scatter_3d"].RGB = image_array

        method = self._method_combobox.currentText()
        self._visuals["spectral_locus"].method = method
        self._visuals["chromaticity_diagram"].method = method
        self._visuals["rgb_color_space_input_2d"].method = method
        self._visuals["rgb_color_space_input_3d"].model = method
        self._visuals["rgb_color_space_chromaticities_2d"].method = method
        self._visuals["rgb_color_space_chromaticities_3d"].model = method
        self._visuals["rgb_scatter_3d"].model = method

        self._wgpu_viewer.render()

    def _reset_camera(self) -> None:
        self._wgpu_viewer.wgpu_camera.fov = 0
        self._wgpu_viewer.wgpu_camera.local.position = np.array([0.5, 0.5, 2])
        self._wgpu_viewer.wgpu_camera.show_pos(np.array([0.5, 0.5, 0.5]))

        self._wgpu_viewer.render()
