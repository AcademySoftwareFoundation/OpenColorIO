# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import enum
from typing import Optional

import numpy as np

import OpenImageIO as oiio
import PyOpenColorIO as ocio
from PySide2 import QtCore, QtGui, QtWidgets
from PySide2.Qt3DCore import Qt3DCore
from PySide2.Qt3DExtras import Qt3DExtras
from PySide2.Qt3DRender import Qt3DRender

from ..constants import R_COLOR, G_COLOR, B_COLOR, GRAY_COLOR
from ..message_router import MessageRouter
from ..utils import float_to_uint8, get_glyph_icon
from ..widgets import ComboBox, EnumComboBox, ExpandingStackedWidget


CUBE_LINES = np.array(
    [
        # fmt: off
        [0.0, 1.0, 0.0], [0.0, 1.0, 1.0],
        [0.0, 1.0, 1.0], [1.0, 1.0, 1.0],
        [1.0, 1.0, 1.0], [1.0, 1.0, 0.0],
        [1.0, 1.0, 0.0], [0.0, 1.0, 0.0],
        [0.0, 1.0, 0.0], [0.0, 0.0, 0.0],
        [0.0, 1.0, 1.0], [0.0, 0.0, 1.0],
        [1.0, 1.0, 1.0], [1.0, 0.0, 1.0],
        [1.0, 1.0, 0.0], [1.0, 0.0, 0.0],
        [0.0, 0.0, 0.0], [0.0, 0.0, 1.0],
        [0.0, 0.0, 1.0], [1.0, 0.0, 1.0],
        [1.0, 0.0, 1.0], [1.0, 0.0, 0.0],
        [1.0, 0.0, 0.0], [0.0, 0.0, 0.0],
        # fmt: on
    ],
    dtype=np.float32,
)
CUBE_VERTEX_COUNT = CUBE_LINES.size // 3


class InputSource(enum.Enum):
    """Enum of CubeView input sources."""

    CUBE = "cube"
    """Plot a 3D cube grid."""

    IMAGE = "image"
    """Plot the currently viewed image."""


class CubeInspector(QtWidgets.QWidget):
    """
    Widget for plotting the current transform's color volume as a
    point cloud, which updates asynchronously when visible.
    """

    FMT_R_LABEL = f'<span style="color:{R_COLOR.name()};">{{v}}</span>'
    FMT_G_LABEL = f'<span style="color:{G_COLOR.name()};">{{v}}</span>'
    FMT_B_LABEL = f'<span style="color:{B_COLOR.name()};">{{v}}</span>'
    FMT_SWATCH_CSS = "background-color: rgb({r}, {g}, {b});"

    @classmethod
    def label(cls) -> str:
        return "Cube"

    @classmethod
    def icon(cls) -> QtGui.QIcon:
        return get_glyph_icon("mdi6.cube-outline")

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        self._sample_format = ""

        # Widgets
        self.input_src_label = get_glyph_icon("mdi6.import", as_widget=True)
        self.input_src_label.setToolTip("Input source")
        self.input_src_box = EnumComboBox(InputSource)
        self.input_src_box.setToolTip(self.input_src_label.toolTip())
        self.input_src_box.setSizeAdjustPolicy(QtWidgets.QComboBox.AdjustToContents)
        self.input_src_box.currentIndexChanged[int].connect(self._on_input_src_changed)

        self.cube_size_label = get_glyph_icon("mdi6.grid", as_widget=True)
        self.cube_size_label.setToolTip("Cube size (edge length)")
        self.cube_size_box = ComboBox()
        self.cube_size_box.setToolTip(self.cube_size_label.toolTip())
        self.cube_size_box.setSizeAdjustPolicy(QtWidgets.QComboBox.AdjustToContents)

        # Add cube size presets
        edge_length = 2
        for i in range(8):
            self.cube_size_box.addItem(
                f"{edge_length}x{edge_length}x{edge_length}", userData=edge_length
            )
            edge_length = edge_length * 2 - 1

        default_index = self.cube_size_box.findData(CubeView.CUBE_SIZE_DEFAULT)
        if default_index != -1:
            self.cube_size_box.setCurrentIndex(default_index)
        self.cube_size_box.currentIndexChanged.connect(self._on_cube_size_changed)

        self.image_detail_label = get_glyph_icon(
            "mdi6.image-size-select-large", as_widget=True
        )
        self.image_detail_label.setToolTip("Image detail (nearest)")
        self.image_detail_box = ComboBox()
        self.image_detail_box.setToolTip(self.image_detail_label.toolTip())
        self.image_detail_box.setSizeAdjustPolicy(QtWidgets.QComboBox.AdjustToContents)

        # Add image detail presets
        ratio = 1
        for i in range(8):
            self.image_detail_box.addItem(f"1:{ratio}", userData=ratio)
            ratio *= 2

        default_index = self.image_detail_box.findData(CubeView.IMAGE_DETAIL_DEFAULT)
        if default_index != -1:
            self.image_detail_box.setCurrentIndex(default_index)
        self.image_detail_box.currentIndexChanged.connect(self._on_image_detail_changed)

        self.show_src_cube_button = QtWidgets.QPushButton()
        self.show_src_cube_button.setFixedHeight(32)
        self.show_src_cube_button.setCheckable(True)
        self.show_src_cube_button.setChecked(True)
        self.show_src_cube_button.setIcon(get_glyph_icon("ph.bounding-box"))
        self.show_src_cube_button.setToolTip("Show source cube outline")
        self.show_src_cube_button.clicked[bool].connect(
            self._on_show_src_cube_button_clicked
        )

        self.clamp_to_src_cube_button = QtWidgets.QPushButton()
        self.clamp_to_src_cube_button.setFixedHeight(32)
        self.clamp_to_src_cube_button.setCheckable(True)
        self.clamp_to_src_cube_button.setChecked(False)
        self.clamp_to_src_cube_button.setIcon(get_glyph_icon("ph.crop"))
        self.clamp_to_src_cube_button.setToolTip("Clamp samples to source cube")
        self.clamp_to_src_cube_button.clicked[bool].connect(
            self._on_clamp_to_src_cube_button_clicked
        )

        self.sample_precision_label = get_glyph_icon(
            "mdi6.decimal-increase", as_widget=True
        )
        self.sample_precision_label.setToolTip(
            "Sample precision (number of digits after the decimal point)"
        )
        self.sample_precision_box = QtWidgets.QSpinBox()
        self.sample_precision_box.setToolTip(self.sample_precision_label.toolTip())
        self.sample_precision_box.setValue(5)

        self.view = CubeView()

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
        cube_size_layout = QtWidgets.QHBoxLayout()
        cube_size_layout.setContentsMargins(0, 0, 0, 0)
        cube_size_layout.addWidget(self.cube_size_label)
        cube_size_layout.addWidget(self.cube_size_box)
        cube_size_frame = QtWidgets.QFrame()
        cube_size_frame.setLayout(cube_size_layout)

        image_detail_layout = QtWidgets.QHBoxLayout()
        image_detail_layout.setContentsMargins(0, 0, 0, 0)
        image_detail_layout.addWidget(self.image_detail_label)
        image_detail_layout.addWidget(self.image_detail_box)
        image_detail_frame = QtWidgets.QFrame()
        image_detail_frame.setLayout(image_detail_layout)

        self.input_stack = ExpandingStackedWidget()
        self.input_stack.addWidget(cube_size_frame)
        self.input_stack.addWidget(image_detail_frame)

        button_layout = QtWidgets.QHBoxLayout()
        button_layout.setSpacing(0)
        button_layout.addWidget(self.show_src_cube_button)
        button_layout.addWidget(self.clamp_to_src_cube_button)

        option_layout = QtWidgets.QHBoxLayout()
        option_layout.addWidget(self.input_src_label)
        option_layout.addWidget(self.input_src_box)
        option_layout.addWidget(self.input_stack)
        option_layout.addStretch()
        option_layout.setStretch(3, 3)
        option_layout.addLayout(button_layout)
        option_layout.addWidget(self.sample_precision_label)
        option_layout.addWidget(self.sample_precision_box)
        option_layout.setStretch(6, 1)

        inspect_layout = QtWidgets.QHBoxLayout()
        inspect_layout.setContentsMargins(8, 8, 8, 8)
        inspect_layout.addStretch()
        inspect_layout.addWidget(self.output_sample_label)
        inspect_layout.addWidget(self.output_r_sample_label)
        inspect_layout.addWidget(self.output_g_sample_label)
        inspect_layout.addWidget(self.output_b_sample_label)
        inspect_layout.addWidget(self.output_sample_swatch)

        self.inspect_bar = QtWidgets.QFrame()
        self.inspect_bar.setObjectName("cube_inspector__status_bar")
        self.inspect_bar.setStyleSheet(
            "QFrame#cube_inspector__status_bar { background-color: black; }"
        )
        self.inspect_bar.setLayout(inspect_layout)

        view_layout = QtWidgets.QVBoxLayout()
        view_layout.setSpacing(0)
        view_layout.addWidget(self.view)
        view_layout.setStretch(0, 1)
        view_layout.addWidget(self.inspect_bar)
        view_layout.setStretch(1, 0)

        layout = QtWidgets.QVBoxLayout()
        layout.addLayout(option_layout)
        layout.setStretch(0, 0)
        layout.addLayout(view_layout)
        layout.setStretch(1, 1)

        self.setLayout(layout)

        # Connect signals/slots
        self.view.sample_changed.connect(self._on_sample_changed)
        self.sample_precision_box.valueChanged.connect(
            self._on_sample_precision_changed
        )

        # Initialize
        self._on_sample_precision_changed(self.sample_precision_box.value())
        self._on_sample_changed(0.0, 0.0, 0.0)

    def reset(self) -> None:
        """Reset samples."""
        self.view.reset()

    @QtCore.Slot(int)
    def _on_input_src_changed(self, index: int) -> None:
        """Update source for plotted samples."""
        self.input_stack.setCurrentIndex(index)
        self.view.set_input_source(self.input_src_box.member())

    @QtCore.Slot(int)
    def _on_cube_size_changed(self, index: int) -> None:
        """Update cube lattice resolution."""
        self.view.set_cube_size(self.cube_size_box.currentData())

    @QtCore.Slot(int)
    def _on_image_detail_changed(self, index: int) -> None:
        """Update the ratio of pixels sampled from the current image."""
        self.view.set_image_detail(self.image_detail_box.currentData())

    @QtCore.Slot(bool)
    def _on_show_src_cube_button_clicked(self, checked: bool) -> None:
        """Toggle whether the source cube edges are visible."""
        self.view.set_show_source_cube(checked)

    @QtCore.Slot(bool)
    def _on_clamp_to_src_cube_button_clicked(self, checked: bool) -> None:
        """Toggle whether to clamp samples to source cube."""
        self.view.set_clamp_to_source_cube(checked)

    @QtCore.Slot(float, float, float)
    def _on_sample_changed(
        self, r_output: float, g_output: float, b_output: float
    ) -> None:
        """Update color value labels from currently hovered sample."""
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

    @QtCore.Slot(int)
    def _on_sample_precision_changed(self, value: float) -> None:
        """Update sample label decimal precision."""
        self._sample_format = f"{{v:.{value}f}}"


class CubeView(QtWidgets.QWidget):
    """Widget for rendering a color volume point cloud."""

    sample_changed = QtCore.Signal(float, float, float)

    CUBE_SIZE_DEFAULT = 33
    IMAGE_DETAIL_DEFAULT = 4

    def __init__(
        self,
        input_src: InputSource = InputSource.CUBE,
        cube_size: int = CUBE_SIZE_DEFAULT,
        image_detail: int = IMAGE_DETAIL_DEFAULT,
        show_src_cube: bool = True,
        clamp_to_src: bool = False,
        parent: Optional[QtWidgets.QWidget] = None,
    ):
        """
        :param input_src: Source for plotted samples
        :param cube_size: Number of samples per cube dimension
        :param image_detail: Ratio of image samples to pixels
        :param show_src_cube: Whether to show source cube edges
        :param clamp_to_src: Whether to clamp samples to source cube
        """
        super().__init__(parent=parent)
        self.setMouseTracking(True)
        self.setSizePolicy(
            QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding
        )
        size = self.size()

        self._input_src = input_src
        self._cube_size = cube_size
        self._image_detail = image_detail
        self._show_src_cube = show_src_cube
        self._clamp_to_src = clamp_to_src

        self._mouse_button = None
        self._mouse_last_pos = None
        self._cpu_proc = None
        self._image_buf: np.ndarray = None
        self._first_show = False

        # Scene
        self.root_entity = Qt3DCore.QEntity()

        self.scene_tf = Qt3DCore.QTransform()
        self.scene_tf.setScale3D(QtGui.QVector3D(10, 10, 10))

        self.material = Qt3DExtras.QPerVertexColorMaterial(self.root_entity)

        # Points
        self.points_color_bytes: QtCore.QByteArray = None
        self.points_color_buf = Qt3DRender.QBuffer()
        self.points_color_attr = Qt3DRender.QAttribute(self.root_entity)
        self.points_color_attr.setBuffer(self.points_color_buf)
        self.points_color_attr.setAttributeType(Qt3DRender.QAttribute.VertexAttribute)
        self.points_color_attr.setName(
            Qt3DRender.QAttribute.defaultColorAttributeName()
        )
        self.points_color_attr.setVertexBaseType(Qt3DRender.QAttribute.Float)
        self.points_color_attr.setVertexSize(3)

        self.points_buf_bytes: QtCore.QByteArray = None
        self.points_pos_buf = Qt3DRender.QBuffer()
        self.points_pos_attr = Qt3DRender.QAttribute(self.root_entity)
        self.points_pos_attr.setBuffer(self.points_pos_buf)
        self.points_pos_attr.setAttributeType(Qt3DRender.QAttribute.VertexAttribute)
        self.points_pos_attr.setName(
            Qt3DRender.QAttribute.defaultPositionAttributeName()
        )
        self.points_pos_attr.setVertexBaseType(Qt3DRender.QAttribute.Float)
        self.points_pos_attr.setVertexSize(3)

        self.points_geo = Qt3DRender.QGeometry()
        self.points_geo.addAttribute(self.points_pos_attr)
        self.points_geo.addAttribute(self.points_color_attr)

        self.points_render = Qt3DRender.QGeometryRenderer(self.root_entity)
        self.points_render.setGeometry(self.points_geo)
        self.points_render.setPrimitiveType(Qt3DRender.QGeometryRenderer.Points)

        self.points_picker = Qt3DRender.QObjectPicker()
        self.points_picker.clicked.connect(self._on_point_clicked)

        self.points_entity = Qt3DCore.QEntity(self.root_entity)
        self.points_entity.addComponent(self.points_render)
        self.points_entity.addComponent(self.scene_tf)
        self.points_entity.addComponent(self.material)
        self.points_entity.addComponent(self.points_picker)

        # Lines
        self.lines_color_bytes: QtCore.QByteArray = None
        self.lines_color_buf = Qt3DRender.QBuffer()
        self.lines_color_attr = Qt3DRender.QAttribute(self.root_entity)
        self.lines_color_attr.setBuffer(self.lines_color_buf)
        self.lines_color_attr.setAttributeType(Qt3DRender.QAttribute.VertexAttribute)
        self.lines_color_attr.setName(Qt3DRender.QAttribute.defaultColorAttributeName())
        self.lines_color_attr.setVertexBaseType(Qt3DRender.QAttribute.Float)
        self.lines_color_attr.setVertexSize(3)

        self.lines_buf_bytes: QtCore.QByteArray = None
        self.lines_pos_buf = Qt3DRender.QBuffer()
        self.lines_pos_attr = Qt3DRender.QAttribute(self.root_entity)
        self.lines_pos_attr.setBuffer(self.lines_pos_buf)
        self.lines_pos_attr.setAttributeType(Qt3DRender.QAttribute.VertexAttribute)
        self.lines_pos_attr.setName(
            Qt3DRender.QAttribute.defaultPositionAttributeName()
        )
        self.lines_pos_attr.setVertexBaseType(Qt3DRender.QAttribute.Float)
        self.lines_pos_attr.setVertexSize(3)

        self.lines_geo = Qt3DRender.QGeometry()
        self.lines_geo.addAttribute(self.lines_pos_attr)
        self.lines_geo.addAttribute(self.lines_color_attr)

        self.lines_render = Qt3DRender.QGeometryRenderer(self.root_entity)
        self.lines_render.setGeometry(self.lines_geo)
        self.lines_render.setPrimitiveType(Qt3DRender.QGeometryRenderer.Lines)

        self.lines_entity = Qt3DCore.QEntity(self.root_entity)
        self.lines_entity.addComponent(self.lines_render)
        self.lines_entity.addComponent(self.scene_tf)
        self.lines_entity.addComponent(self.material)

        # Widgets
        self._window = Qt3DExtras.Qt3DWindow()
        self._window.setRootEntity(self.root_entity)
        self._window.installEventFilter(self)

        self.view = QtWidgets.QWidget.createWindowContainer(self._window, self)
        self.view.resize(size)

        self.installEventFilter(self)

        # Camera
        camera = self._window.camera()
        camera.lens().setPerspectiveProjection(
            45.0, size.width() / size.height(), 0.1, 1000.0
        )
        camera.setPosition(QtGui.QVector3D(15, 15, 15))
        camera.setViewCenter(QtGui.QVector3D(5, 5, 5))

        # Render settings
        render_settings = self._window.renderSettings()

        self.picking_settings = render_settings.pickingSettings()
        self.picking_settings.setPickMethod(Qt3DRender.QPickingSettings.PointPicking)
        self.picking_settings.setPickResultMode(Qt3DRender.QPickingSettings.NearestPick)
        self.picking_settings.setFaceOrientationPickingMode(
            Qt3DRender.QPickingSettings.FrontAndBackFace
        )

        self.depth_test = Qt3DRender.QDepthTest()
        self.depth_test.setDepthFunction(Qt3DRender.QDepthTest.Less)

        self.point_size = Qt3DRender.QPointSize()
        self.point_size.setSizeMode(Qt3DRender.QPointSize.Fixed)

        self.line_width = Qt3DRender.QLineWidth()
        self.line_width.setSmooth(True)
        self.line_width.setValue(2.0)

        self.render_state_set = Qt3DRender.QRenderStateSet()
        self.render_state_set.addRenderState(self.depth_test)
        self.render_state_set.addRenderState(self.point_size)
        self.render_state_set.addRenderState(self.line_width)

        self.forward_renderer = Qt3DExtras.QForwardRenderer(self.depth_test)
        self.forward_renderer.setClearColor(QtGui.QColor(QtCore.Qt.black))
        self.forward_renderer.setCamera(camera)

        render_settings.setActiveFrameGraph(self.render_state_set)

        # Layout
        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.view)

        # Initialize
        self._update_cube()
        self._update_point_size()
        self._update_picker_tolerance()

        msg_router = MessageRouter.get_instance()
        msg_router.processor_ready.connect(self._on_processor_ready)
        msg_router.image_ready.connect(self._on_image_ready)

    def showEvent(self, event: QtGui.QShowEvent) -> None:
        """Start listening for processor updates, if visible."""
        super().showEvent(event)

        msg_router = MessageRouter.get_instance()
        msg_router.set_processor_updates_allowed(True)
        msg_router.set_image_updates_allowed(True)

        if not self._first_show:
            self.fit()
            self._first_show = True

    def hideEvent(self, event: QtGui.QHideEvent) -> None:
        """Stop listening for processor updates, if not visible."""
        super().hideEvent(event)

        msg_router = MessageRouter.get_instance()
        msg_router.set_processor_updates_allowed(False)
        msg_router.set_image_updates_allowed(False)

    def resizeEvent(self, event: QtGui.QResizeEvent) -> None:
        """
        Re-fit 3D window and its camera projection matrix to widget
        size.
        """
        super().resizeEvent(event)
        size = self.size()

        # Fit 3D window to widget size
        self.view.resize(size)

        # Fit projection matrix aspect ratio to window
        self._window.camera().lens().setAspectRatio(size.width() / size.height())

    def eventFilter(self, watched: QtCore.QObject, event: QtCore.QEvent) -> bool:
        """Camera control implementation."""
        if watched == self._window:
            event_type = event.type()

            # Keyboard shortcuts
            if event_type == QtCore.QEvent.KeyRelease:
                key = event.key()
                if key == QtCore.Qt.Key_F:
                    self.fit()
                    return True

            # Mouse button press
            elif event_type == QtCore.QEvent.MouseButtonPress:
                self._mouse_button = event.button()
                self._mouse_last_pos = event.pos()
                return False

            # Mouse button release
            elif event_type == QtCore.QEvent.MouseButtonRelease:
                if event.button() == self._mouse_button:
                    self._mouse_button = None
                    return False

            elif event_type == QtCore.QEvent.MouseMove:
                pos = QtCore.QPointF(event.pos())

                # Mouse move (orbit)
                if self._mouse_button == QtCore.Qt.LeftButton:
                    if self._mouse_last_pos is not None:
                        camera = self._window.camera()
                        delta = (pos - self._mouse_last_pos) * 0.25

                        # Rotate around cube
                        camera.panAboutViewCenter(-delta.x())

                        # Tilt up/down, but stay clear of singularities directly above
                        # or below view center.
                        camera.tiltAboutViewCenter(delta.y())
                        if 1 - abs(camera.viewVector().normalized().y()) <= 0.01:
                            # Revert tilt change
                            camera.tiltAboutViewCenter(-delta.y())

                        # Make sure the camera is always upright
                        camera.setUpVector(QtGui.QVector3D(0, 1, 0))

                self._mouse_last_pos = pos

                return True

            # Mouse wheel (zoom)
            elif event_type == QtCore.QEvent.Wheel:
                camera = self._window.camera()
                delta = np.sign(event.delta()) * 0.25

                # Zoom, but keep back from view center to prevent flipping the view
                camera.translate(
                    QtGui.QVector3D(0, 0, delta),
                    option=Qt3DRender.QCamera.DontTranslateViewCenter,
                )
                camera_dist = camera.position().distanceToPoint(camera.viewCenter())
                if camera_dist < 2:
                    # Revert zoom change
                    camera.translate(
                        QtGui.QVector3D(0, 0, -delta),
                        option=Qt3DRender.QCamera.DontTranslateViewCenter,
                    )

                self._update_point_size()
                return True

        return False

    def reset(self) -> None:
        """Reset samples."""
        self._cpu_proc = None
        self._update_cube()

    def set_input_source(self, input_src: InputSource) -> None:
        """
        Set the source for plotted samples.

        :param input_src: Source for plotted samples
        """
        if input_src != self._input_src:
            self._input_src = input_src
            self._update_cube()

    def set_cube_size(self, cube_size: int) -> None:
        """
        Set cube lattice resolution.

        :param cube_size: Number of samples per cube dimension
        """
        if cube_size != self._cube_size:
            self._cube_size = cube_size
            self._update_picker_tolerance()
            self._update_cube()

    def set_image_detail(self, image_detail: int) -> None:
        """
        Set the ratio of pixels sampled from the current image.

        :param image_detail: Ratio of image samples to pixels
        """
        if image_detail != self._image_detail:
            self._image_detail = image_detail
            self._update_cube()

    def set_show_source_cube(self, show: bool) -> None:
        """
        Set whether to draw the source cube edges in the scene.

        :param show: Whether to show source cube
        """
        if show != self._show_src_cube:
            self._show_src_cube = show
            self._update_cube()

    def set_clamp_to_source_cube(self, clamp: bool) -> None:
        """
        Set whether to clamp samples that fall outside the source cube.

        :param clamp: Whether to clamp samples tp source cube
        """
        if clamp != self._clamp_to_src:
            self._clamp_to_src = clamp
            self._update_cube()

    def fit(self) -> None:
        """Fit cube to viewport."""
        camera = self._window.camera()
        camera.viewEntity(self.lines_entity)

    def _update_point_size(self):
        """Update point size relative to camera distance."""
        camera = self._window.camera()
        camera_dist = camera.position().distanceToPoint(camera.viewCenter())

        # Dynamic point size. Linearly map [100, 0] camera distance range to
        # [1, 10] point size range.
        amount = min(1.0, max(0.0, (100 - camera_dist) / 100))
        point_size = 1 + (10 - 1) * amount
        self.point_size.setValue(point_size)

    def _update_picker_tolerance(self):
        """
        Update point picker tolerance relative to cube size, providing
        each point a hit area adjacent to its neighboring points.
        """
        #   o-----o-----o    cube_size points
        #   o--o--o--o--o    cube_size+1 sections
        # --o--|--o--|--o--  tolerance == 2x section length, centered on vertex
        self.picking_settings.setWorldSpaceTolerance((10.0 / (self._cube_size + 1)) * 2)

    def _update_cube(self) -> None:
        """
        Update cube point cloud and lines from parameters and transform
        data, if available.
        """
        # Points
        if self._input_src == InputSource.IMAGE and self._image_buf is not None:
            image_spec = self._image_buf.spec()
            identity_grid = self._image_buf.get_pixels(
                oiio.FLOAT,
                roi=oiio.ROI(
                    # fmt: off
                    image_spec.x,
                    image_spec.x + image_spec.width,
                    image_spec.y,
                    image_spec.y + image_spec.height,
                    0, 1,
                    0, 3,
                    # fmt: on
                ),
            ).reshape((image_spec.width * image_spec.height, 3))[:: self._image_detail]
            sample_count = identity_grid.size // 3
        else:
            sample_count = self._cube_size**3
            sample_range = np.linspace(0.0, 1.0, self._cube_size, dtype=np.float32)
            identity_grid = (
                np.stack(np.meshgrid(sample_range, sample_range, sample_range))
                .transpose()
                .reshape((sample_count, 3))
            )

        tf_grid = identity_grid.copy()

        if self._cpu_proc is not None:
            # Apply processor to identity grid
            self._cpu_proc.applyRGB(tf_grid)

        if self._clamp_to_src:
            tf_r = tf_grid[:, 0]
            tf_g = tf_grid[:, 1]
            tf_b = tf_grid[:, 2]

            tf_r_clamp = np.argwhere(np.logical_or(tf_r < 0.0, tf_r > 1.0))
            tf_g_clamp = np.argwhere(np.logical_or(tf_g < 0.0, tf_g > 1.0))
            tf_b_clamp = np.argwhere(np.logical_or(tf_b < 0.0, tf_b > 1.0))
            tf_clamp = np.unique(
                np.concatenate((tf_r_clamp.flat, tf_g_clamp.flat, tf_b_clamp.flat))
            )

            tf_samples = np.delete(tf_grid, tf_clamp, axis=0)
            points_vertex_count = tf_samples.size // 3
        else:
            tf_samples = tf_grid
            points_vertex_count = sample_count

        # Clamp colors for display
        tf_colors = np.clip(tf_samples, 0.0, 1.0)

        self.points_color_bytes = QtCore.QByteArray(tf_colors.tobytes())
        self.points_color_buf.setData(self.points_color_bytes)
        self.points_color_attr.setCount(points_vertex_count)

        self.points_pos_bytes = QtCore.QByteArray(tf_samples.tobytes())
        self.points_pos_buf.setData(self.points_pos_bytes)
        self.points_pos_attr.setCount(points_vertex_count)

        self.points_render.setVertexCount(points_vertex_count)

        # Lines
        if self._show_src_cube:
            self.lines_color_bytes = QtCore.QByteArray(CUBE_LINES.tobytes())
            self.lines_color_buf.setData(self.lines_color_bytes)
            self.lines_color_attr.setCount(CUBE_VERTEX_COUNT)

            self.lines_pos_bytes = QtCore.QByteArray(CUBE_LINES.tobytes())
            self.lines_pos_buf.setData(self.lines_pos_bytes)
            self.lines_pos_attr.setCount(CUBE_VERTEX_COUNT)

            self.lines_render.setVertexCount(CUBE_VERTEX_COUNT)
        else:
            self.lines_color_attr.setCount(0)
            self.lines_pos_attr.setCount(0)
            self.lines_render.setVertexCount(0)

        self.update()

    @QtCore.Slot(np.ndarray)
    def _on_image_ready(self, image_buf: oiio.ImageBuf) -> None:
        """
        Update point cloud from image buffer.

        :param image_buf: Current image buffer
        """
        self._image_buf = image_buf
        if self._input_src == InputSource.IMAGE:
            self._update_cube()

    @QtCore.Slot(ocio.CPUProcessor)
    def _on_processor_ready(self, cpu_proc: ocio.CPUProcessor) -> None:
        """
        Update point cloud transform from OCIO CPU processor.

        :param cpu_proc: CPU processor of current transform
        """
        self._cpu_proc = cpu_proc
        self._update_cube()

    def _on_point_clicked(self, pick: Qt3DRender.QPickPointEvent) -> None:
        """Triggered on sample point click."""
        if self._first_show:
            color = pick.localIntersection()
            self.sample_changed.emit(color.x(), color.y(), color.z())
