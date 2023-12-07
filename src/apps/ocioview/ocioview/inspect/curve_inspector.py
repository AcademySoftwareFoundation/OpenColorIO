# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import enum
import math
from typing import Optional

import numpy as np

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import R_COLOR, G_COLOR, B_COLOR, GRAY_COLOR
from ..message_router import MessageRouter
from ..utils import get_glyph_icon, SignalsBlocked
from ..widgets import EnumComboBox, FloatEditArray, IntEdit


class SampleType(enum.Enum):
    """Enum of curve sampling types for representing OCIO transforms."""

    LINEAR = "linear"
    """Linear scale."""

    LOG = "log"
    """Log scale with a user-defined base."""


class CurveInspector(QtWidgets.QWidget):
    """
    Widget for inspecting OCIO transform tone curves, which updates
    asynchronously when visible, to reduce unnecessary background
    processing.
    """

    @classmethod
    def label(cls) -> str:
        return "Curve"

    @classmethod
    def icon(cls) -> QtGui.QIcon:
        return get_glyph_icon("mdi6.chart-bell-curve-cumulative")

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Widgets
        self.input_range_label = get_glyph_icon("mdi6.import", as_widget=True)
        self.input_range_label.setToolTip("Input range")
        self.input_range_edit = FloatEditArray(
            labels=["min", "max"],
            defaults=[CurveView.INPUT_MIN_DEFAULT, CurveView.INPUT_MAX_DEFAULT],
        )
        self.input_range_edit.setToolTip(self.input_range_label.toolTip())
        self.input_range_edit.value_changed.connect(self._on_input_range_changed)

        self.sample_count_label = get_glyph_icon("ph.line-segments", as_widget=True)
        self.sample_count_label.setToolTip("Sample count")
        self.sample_count_edit = IntEdit(default=CurveView.SAMPLE_COUNT_DEFAULT)
        self.sample_count_edit.setToolTip(self.sample_count_label.toolTip())
        self.sample_count_edit.value_changed.connect(self._on_sample_count_changed)

        self.sample_type_label = get_glyph_icon("mdi6.function-variant", as_widget=True)
        self.sample_type_label.setToolTip("Sample type")
        self.sample_type_combo = EnumComboBox(SampleType)
        self.sample_type_combo.setToolTip(self.sample_type_label.toolTip())
        self.sample_type_combo.currentIndexChanged[int].connect(
            self._on_sample_type_changed
        )

        self.log_base_label = get_glyph_icon("mdi6.math-log", as_widget=True)
        self.log_base_label.setToolTip("Log base")
        self.log_base_edit = IntEdit(default=CurveView.LOG_BASE_DEFAULT)
        self.log_base_edit.setToolTip(self.log_base_label.toolTip())
        self.log_base_edit.value_changed.connect(self._on_log_base_changed)
        self.log_base_edit.setEnabled(False)

        self.view = CurveView()

        # Layout
        option_layout = QtWidgets.QHBoxLayout()
        option_layout.addWidget(self.input_range_label)
        option_layout.addWidget(self.input_range_edit)
        option_layout.setStretch(1, 3)
        option_layout.addWidget(self.sample_count_label)
        option_layout.addWidget(self.sample_count_edit)
        option_layout.setStretch(3, 2)
        option_layout.addWidget(self.sample_type_label)
        option_layout.addWidget(self.sample_type_combo)
        option_layout.setStretch(5, 3)
        option_layout.addWidget(self.log_base_label)
        option_layout.addWidget(self.log_base_edit)
        option_layout.setStretch(7, 2)

        layout = QtWidgets.QVBoxLayout()
        layout.addLayout(option_layout)
        layout.addWidget(self.view)

        self.setLayout(layout)

    def reset(self) -> None:
        """Clear rendered curves."""
        self.view.reset()

    @QtCore.Slot(str, float)
    def _on_input_range_changed(self, label: str, value: float) -> None:
        """
        Triggered when the user changes the input range, which defines
        the input values to be sampled through the transform.

        :param label: Float edit label
        :param value: Float edit value
        """
        self.view.set_input_range(
            self.input_range_edit.component_value("min"),
            self.input_range_edit.component_value("max"),
        )

    @QtCore.Slot(int)
    def _on_sample_count_changed(self, sample_count: int) -> None:
        """
        Triggered when the user changes the number of samples to be
        processed within the input range through the transform.

        :param sample_count: Number of samples. Typically, a power of 2
            number.
        """
        if sample_count >= CurveView.SAMPLE_COUNT_MIN:
            self.view.set_sample_count(sample_count)
        else:
            with SignalsBlocked(self.sample_count_edit):
                self.sample_count_edit.set_value(CurveView.SAMPLE_COUNT_MIN)
                self.view.set_sample_count(CurveView.SAMPLE_COUNT_MIN)

    @QtCore.Slot(int)
    def _on_sample_type_changed(self, index: int) -> None:
        """
        Triggered when the user changes the sample type, which defines
        how samples are distributed within the input range.

        :param index: Curve type index
        """
        sample_type = self.sample_type_combo.member()
        self.log_base_edit.setEnabled(sample_type == SampleType.LOG)
        self.view.set_sample_type(sample_type)

    @QtCore.Slot(int)
    def _on_log_base_changed(self, log_base: int) -> None:
        """
        Triggered when the user changes the base for the log sample
        type.

        :param log_base: Log scale base
        """
        self.view.set_log_base(log_base)


class CurveView(QtWidgets.QGraphicsView):
    """
    Widget for rendering OCIO transform tone curves.
    """

    SAMPLE_COUNT_MIN = 2**8
    SAMPLE_COUNT_DEFAULT = 2**10
    INPUT_MIN_DEFAULT = 0.0
    INPUT_MAX_DEFAULT = 1.0
    LOG_BASE_DEFAULT = 2
    CURVE_SCALE = 100
    FONT_HEIGHT = 4

    # The curve viewer only shows 5 digit decimal precision, so this should work fine
    # as a minimum when input min is 0.
    EPSILON = 1e-5

    def __init__(
        self,
        input_min: float = INPUT_MIN_DEFAULT,
        input_max: float = INPUT_MAX_DEFAULT,
        sample_count: int = SAMPLE_COUNT_DEFAULT,
        sample_type: SampleType = SampleType.LINEAR,
        log_base: int = LOG_BASE_DEFAULT,
        parent: Optional[QtWidgets.QWidget] = None,
    ):
        """
        :param input_min: Input range minimum value
        :param input_max: Input range maximum value
        :param sample_count: Number of samples in the input range,
            which will typically be a power of 2 value.
        :param sample_type: Sample scale/distribution type
        :param log_base: Log scale base when sample_type is
            `SampleType.LOG`.
        """
        super().__init__(parent=parent)
        self.setRenderHint(QtGui.QPainter.Antialiasing, True)
        self.setViewportUpdateMode(QtWidgets.QGraphicsView.FullViewportUpdate)
        self.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setMouseTracking(True)
        self.scale(1, -1)  # Flip to make origin the bottom-left corner

        # Input  range
        if input_max <= input_min:
            input_min = self.INPUT_MIN_DEFAULT
            input_max = self.INPUT_MAX_DEFAULT

        self._input_min = input_min
        self._input_max = input_max

        # Sample characteristics
        self._sample_count = max(self.SAMPLE_COUNT_MIN, sample_count)
        self._sample_type = sample_type
        self._log_base = max(2, log_base)

        # Cached sample data
        self._sample_ellipse: Optional[QtWidgets.QGraphicsEllipseItem] = None
        self._sample_text: Optional[QtWidgets.QGraphicsTextItem] = None
        self._sample_rect: Optional[QtCore.QRectF] = None

        self._samples: dict[str, np.ndarray] = {}
        self._nearest_samples: dict[str, tuple[float, float, float]] = {}

        self._x_lin: np.ndarray = np.array([], dtype=np.float32)
        self._x_log: np.ndarray = np.array([], dtype=np.float32)
        self._x_min: float = self._input_min
        self._x_max: float = self._input_max
        self._y_min: float = self._input_min
        self._y_max: float = self._input_max

        # Cached curve data
        self._curve_init = False
        self._curve_tf = QtGui.QTransform()
        self._curve_tf_inv = QtGui.QTransform()
        self._curve_paths: dict[str, QtGui.QPainterPath] = {}
        self._curve_items: dict[str, QtWidgets.QGraphicsPathItem] = {}
        self._curve_rect = QtCore.QRectF(
            self._input_min,
            self._input_min,
            self._input_max - self._input_min,
            self._input_max - self._input_min,
        )

        # Cached processor from which the OCIO transform is derived
        self._prev_cpu_proc = None

        # Graphics scene
        self._scene = QtWidgets.QGraphicsScene()
        self.setScene(self._scene)

        # Initialize
        self._update_x_samples()
        msg_router = MessageRouter.get_instance()
        msg_router.cpu_processor_ready.connect(self._on_cpu_processor_ready)

    def showEvent(self, event: QtGui.QShowEvent) -> None:
        """Start listening for processor updates, if visible."""
        super().showEvent(event)

        msg_router = MessageRouter.get_instance()
        msg_router.set_cpu_processor_updates_allowed(True)

    def hideEvent(self, event: QtGui.QHideEvent) -> None:
        """Stop listening for processor updates, if not visible."""
        super().hideEvent(event)

        msg_router = MessageRouter.get_instance()
        msg_router.set_cpu_processor_updates_allowed(False)

    def resizeEvent(self, event: QtGui.QResizeEvent) -> None:
        """Re-fit graph on resize, to always be centered."""
        super().resizeEvent(event)
        self._fit()

    def mouseMoveEvent(self, event: QtGui.QMouseEvent) -> None:
        """Find nearest samples to current mouse position."""
        super().mouseMoveEvent(event)

        if not self._curve_init:
            return

        self._nearest_samples.clear()

        pos = self._curve_tf_inv.map(self.mapToScene(event.pos()))
        pos_arr = np.array([pos.x(), pos.y()], dtype=np.float32)

        for color_name, samples in self._samples.items():
            all_dist = np.linalg.norm(samples - pos_arr, axis=1)
            nearest_dist_index = np.argmin(all_dist)
            self._nearest_samples[color_name] = (
                (
                    self._x_lin[nearest_dist_index]
                    if self._sample_type == SampleType.LINEAR
                    else self._x_log[nearest_dist_index]
                ),
                samples[nearest_dist_index][0],
                samples[nearest_dist_index][1],
            )

        self._invalidate()

    def wheelEvent(self, event: QtGui.QWheelEvent) -> None:
        """
        Ignore wheel events to prevent scrolling around graphics scene.
        """
        event.ignore()

    def reset(self) -> None:
        """Clear all curves."""
        self._samples.clear()
        self._nearest_samples.clear()

        self._curve_init = False
        self._curve_paths.clear()
        self._curve_items.clear()

        self._prev_cpu_proc = None

        self._scene.clear()
        self._invalidate()

    def set_input_range(self, input_min: float, input_max: float) -> None:
        """
        Set the input range, which defines the input values to be
        sampled through the transform.

        :param input_min: Input range minimum value
        :param input_max: Input range maximum value
        """
        if (
            input_min != self._input_min or input_max != self._input_max
        ) and input_max > input_min:
            self._input_min = input_min
            self._input_max = input_max
            self._update_curves()

    def set_input_min(self, input_min: float) -> None:
        """
        Set the input minimum, which defines the lowest value to be
        sampled through the transform.

        :param input_min: Input range minimum value
        """
        if input_min != self._input_min and input_min < self._input_max:
            self._input_min = input_min
            self._update_curves()

    def set_input_max(self, input_max: float) -> None:
        """
        Set the input maximum, which defines the highest value to be
        sampled through the transform.

        :param input_max: Input range minimum value
        """
        if input_max != self._input_max and input_max > self._input_min:
            self._input_max = input_max
            self._update_curves()

    def set_sample_count(self, sample_count: int) -> None:
        """
        Set the number of samples to be processed within the input
        range through the transform.

        :param sample_count: Number of samples. Typically, a power of 2
            number.
        """
        if sample_count != self._sample_count and sample_count >= self.SAMPLE_COUNT_MIN:
            self._sample_count = sample_count
            self._update_curves()

    def set_sample_type(self, sample_type: SampleType) -> None:
        """
        Set the sample type, which defines how samples are distributed
        within the input range.

        :param sample_type: Curve type index
        """
        if sample_type != self._sample_type:
            self._sample_type = sample_type
            self._update_curves()

    def set_log_base(self, log_base: int) -> None:
        """
        Set the base for the log sample type.

        :param log_base: Log scale base
        """
        if log_base != self._log_base and log_base >= 2:
            self._log_base = log_base
            self._update_curves()

    def drawBackground(self, painter: QtGui.QPainter, rect: QtCore.QRectF) -> None:
        """Draw curve grid and axis values."""
        # Flood fill background
        painter.setPen(QtCore.Qt.NoPen)
        painter.setBrush(QtCore.Qt.black)
        painter.drawRect(rect)

        if not self._curve_init:
            return

        font = painter.font()
        font.setPixelSize(self.FONT_HEIGHT)
        painter.setFont(font)

        text_pen = QtGui.QPen(GRAY_COLOR)
        grid_pen = QtGui.QPen(GRAY_COLOR.darker(200))
        grid_pen.setWidthF(0)

        painter.setPen(grid_pen)
        painter.setBrush(QtCore.Qt.NoBrush)

        # Draw grid border
        painter.drawRect(self._curve_rect)

        # Draw grid rows
        y_text_origin = QtGui.QTextOption(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
        y_text_origin.setWrapMode(QtGui.QTextOption.NoWrap)

        for i, y in enumerate(
            np.linspace(self._y_min, self._y_max, 11, dtype=np.float32)
        ):
            p1 = self._curve_tf.map(QtCore.QPointF(self._x_min, y))
            p2 = self._curve_tf.map(QtCore.QPointF(self._x_max, y))

            if self._y_min < y < self._y_max:
                painter.setPen(grid_pen)
                painter.drawLine(QtCore.QLineF(p1, p2))

            if y > self._y_min:
                label_value = round(y, 2)
                if label_value == 0.0:
                    label_value = abs(label_value)

                painter.save()
                painter.translate(p1)
                painter.scale(1, -1)
                painter.setPen(text_pen)
                painter.drawText(
                    QtCore.QRectF(-42.5, -10, 40, 20), str(label_value), y_text_origin
                )
                painter.restore()

        # Draw grid columns
        x_text_origin = QtGui.QTextOption(QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter)
        x_text_origin.setWrapMode(QtGui.QTextOption.NoWrap)

        sample_step = math.ceil(self._sample_count / 10.0)

        for i, x in enumerate(self._x_lin):
            if not (i % sample_step == 0 or i == self._sample_count - 1):
                continue

            p1 = self._curve_tf.map(QtCore.QPointF(x, self._y_min))
            p2 = self._curve_tf.map(QtCore.QPointF(x, self._y_max))

            if self._x_min < x < self._x_max:
                painter.setPen(grid_pen)
                painter.drawLine(QtCore.QLineF(p1, p2))

            if x > self._x_min:
                label_value = round(
                    x if self._sample_type == SampleType.LINEAR else self._x_log[i],
                    2 if self._sample_type == SampleType.LINEAR else 5,
                )
                if label_value == 0.0:
                    label_value = abs(label_value)

                painter.save()
                painter.translate(p1)
                painter.scale(1, -1)
                painter.rotate(90)
                painter.setPen(text_pen)
                painter.drawText(
                    QtCore.QRectF(2.5 + 1, -10, 40, 20), str(label_value), x_text_origin
                )
                painter.restore()

    def drawForeground(self, painter: QtGui.QPainter, rect: QtCore.QRectF) -> None:
        """Draw nearest sample point and coordinates."""
        if not self._curve_init:
            return

        font = painter.font()
        font.setPixelSize(self.FONT_HEIGHT)
        painter.setFont(font)

        text_origin = QtGui.QTextOption(QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter)
        text_origin.setWrapMode(QtGui.QTextOption.NoWrap)

        sample_l = sample_t = None
        if self._sample_rect is not None:
            sample_l = self._sample_rect.left()
            sample_t = self._sample_rect.top()

        for i, (color_name, nearest_sample) in enumerate(
            reversed(self._nearest_samples.items())
        ):
            # Draw sample point on curve
            painter.setPen(QtCore.Qt.NoPen)
            painter.setBrush(QtGui.QColor(color_name))

            painter.drawEllipse(
                self._curve_tf.map(
                    QtCore.QPointF(
                        nearest_sample[1],
                        nearest_sample[2],
                    )
                ),
                1.5,
                1.5,
            )

            # Draw sample values
            if sample_l is not None:
                painter.setBrush(QtCore.Qt.NoBrush)

                x_label_value = f"{nearest_sample[0]:.05f}"
                y_label_value = f"{nearest_sample[2]:.05f}"

                painter.save()
                painter.translate(QtCore.QPointF(sample_l, sample_t + (20 * i)))
                painter.scale(1, -1)

                painter.setPen(GRAY_COLOR)
                painter.drawText(QtCore.QRectF(0, -20, 5, 10), "X:", text_origin)
                painter.drawText(QtCore.QRectF(0, -10, 5, 10), "Y:", text_origin)

                if color_name == GRAY_COLOR.name():
                    palette = self.palette()
                    painter.setPen(palette.color(palette.ColorRole.Text))
                else:
                    painter.setPen(QtGui.QColor(color_name))

                painter.drawText(
                    QtCore.QRectF(5, -20, 35, 10), x_label_value, text_origin
                )
                painter.drawText(
                    QtCore.QRectF(5, -10, 35, 10), y_label_value, text_origin
                )
                painter.restore()

    def _invalidate(self) -> None:
        """Force repaint of visible region of graphics scene."""
        self._scene.invalidate(QtCore.QRectF(self.visibleRegion().boundingRect()))

    def _update_curves(self) -> None:
        """
        Update X-axis samples and rebuild curves from any cached
        processor.
        """
        self._update_x_samples()
        if self._prev_cpu_proc is not None:
            self._on_cpu_processor_ready(self._prev_cpu_proc)

    def _update_x_samples(self):
        """
        Update linear and log X-axis samples from input and sample
        parameters.
        """
        self._x_lin = np.linspace(
            self._input_min, self._input_max, self._sample_count, dtype=np.float32
        )

        log_min = math.log(max(self.EPSILON, self._input_min))
        log_max = max(log_min + 0.00001, math.log(self._input_max, self._log_base))
        self._x_log = np.logspace(
            log_min, log_max, self._sample_count, base=self._log_base, dtype=np.float32
        )

        self._x_min = self._x_lin.min()
        self._x_max = self._x_lin.max()

    def _fit(self) -> None:
        """Fit and center graph."""
        if not self._curve_init:
            return

        # Use font metrics to calculate text padding, based on estimated maximum
        # number lengths.
        font = self.font()
        font.setPixelSize(self.FONT_HEIGHT)
        fm = QtGui.QFontMetrics(font)

        text_rect = QtCore.QRect(0, 0, 100, 10)
        text_flags = QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter

        pad_b = 15
        pad_t = (
            fm.boundingRect(
                text_rect,
                text_flags,
                "100.01" if self._sample_type == SampleType.LINEAR else "100.00001",
            ).width()
            + 10
        )
        pad_l = fm.boundingRect(text_rect, text_flags, "100.01").width() + 10
        pad_r = fm.boundingRect(text_rect, text_flags, "X: 100.00001").width() + 10

        fit_rect = self._curve_rect.adjusted(-pad_l, -pad_t, pad_r, pad_b)

        # Fit and center on calculated rectangle
        self.fitInView(fit_rect, QtCore.Qt.KeepAspectRatio)
        self.centerOn(fit_rect.center())
        self.update()

    @QtCore.Slot(ocio.CPUProcessor)
    def _on_cpu_processor_ready(self, cpu_proc: ocio.CPUProcessor) -> None:
        """
        Update curves from sampled OCIO CPU processor.

        :param cpu_proc: CPU processor of currently viewed transform
        """
        self.reset()

        self._prev_cpu_proc = cpu_proc

        # Get input samples
        if self._sample_type == SampleType.LOG:
            x_samples = self._x_log
        else:  # LINEAR
            x_samples = self._x_lin

        # Interleave samples per channel
        rgb_samples = np.repeat(x_samples, 3)

        # Apply processor to samples
        cpu_proc.applyRGB(rgb_samples)

        # De-interleave transformed samples
        r_samples = rgb_samples[0::3]
        g_samples = rgb_samples[1::3]
        b_samples = rgb_samples[2::3]

        # Collect sample pairs and min/max Y sample values
        if np.allclose(r_samples, g_samples, atol=self.EPSILON) and np.allclose(
            r_samples, b_samples, atol=self.EPSILON
        ):
            palette = self.palette()
            color_name = palette.color(palette.ColorRole.Text).name()

            self._samples[color_name] = np.stack((self._x_lin, r_samples), axis=-1)

            self._y_min = r_samples.min()
            self._y_max = r_samples.max()

        else:
            y_min = None
            y_max = None

            for i, (color, channel_samples) in enumerate(
                [
                    (R_COLOR, r_samples),
                    (G_COLOR, g_samples),
                    (B_COLOR, b_samples),
                ]
            ):
                color_name = color.name()

                self._samples[color_name] = np.stack(
                    (self._x_lin, channel_samples), axis=-1
                )

                channel_y_min = channel_samples.min()
                if y_min is None or channel_y_min < y_min:
                    y_min = channel_y_min
                channel_y_max = channel_samples.max()
                if y_max is None or channel_y_max > y_max:
                    y_max = channel_y_max

            self._y_min = y_min
            self._y_max = y_max

        # Transform to scale/translate curve so that it fits in a square and
        # has its origin at (0, 0).
        curve_tf = QtGui.QTransform()
        curve_tf.translate(-self._x_min, -self._y_min)
        curve_tf.scale(
            self.CURVE_SCALE / (self._x_max - self._x_min),
            self.CURVE_SCALE / (self._y_max - self._y_min),
        )
        self._curve_tf = curve_tf
        self._curve_tf_inv, ok = curve_tf.inverted()

        # Curve square
        self._curve_rect = QtCore.QRectF(
            self._curve_tf.map(QtCore.QPointF(self._x_min, self._y_min)),
            self._curve_tf.map(QtCore.QPointF(self._x_max, self._y_max)),
        )

        # Sample box rect
        self._sample_rect = QtCore.QRectF(
            self._curve_rect.right() + 5,
            self._curve_rect.top(),
            40,
            len(self._samples) * 20,
        )

        # Build painter paths that fit in square from sample data
        for color_name, channel_samples in self._samples.items():
            curve = QtGui.QPainterPath(
                self._curve_tf.map(
                    QtCore.QPointF(channel_samples[0][0], channel_samples[0][1])
                )
            )
            curve.reserve(channel_samples.shape[0])
            for i in range(1, channel_samples.shape[0]):
                curve.lineTo(
                    self._curve_tf.map(
                        QtCore.QPointF(channel_samples[i][0], channel_samples[i][1])
                    )
                )
            self._curve_paths[color_name] = curve

        # Add curve(s) to scene
        for color_name, curve in self._curve_paths.items():
            pen = QtGui.QPen()
            pen.setColor(QtGui.QColor(color_name))
            pen.setWidthF(0.5)

            curve_item = self._scene.addPath(
                curve, pen, QtGui.QBrush(QtCore.Qt.NoBrush)
            )
            self._curve_items[color_name] = curve_item

        self._curve_init = True

        # Expand scene rect to fit graph
        max_dim = max(self._curve_rect.width(), self._curve_rect.height()) * 2
        scene_rect = self._curve_rect.adjusted(-max_dim, -max_dim, max_dim, max_dim)
        self.setSceneRect(scene_rect)

        self._fit()
