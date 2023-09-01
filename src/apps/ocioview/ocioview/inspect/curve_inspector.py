# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import enum
import math
from typing import Optional

import numpy as np

import PyOpenColorIO as ocio
from PySide2 import QtCore, QtGui, QtWidgets

from ..constants import R_COLOR, G_COLOR, B_COLOR, GRAY_COLOR
from ..message_router import MessageRouter
from ..utils import get_glyph_icon, SignalsBlocked
from ..widgets import EnumComboBox, FloatEditArray, IntEdit


class SampleType(enum.Enum):
    LINEAR = "linear"
    LOG = "log"


class CurveInspector(QtWidgets.QWidget):
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

        self.sample_size_label = get_glyph_icon("ph.line-segments", as_widget=True)
        self.sample_size_label.setToolTip("Sample size")
        self.sample_size_edit = IntEdit(default=CurveView.SAMPLE_SIZE_DEFAULT)
        self.sample_size_edit.setToolTip(self.sample_size_label.toolTip())
        self.sample_size_edit.value_changed.connect(self._on_sample_size_changed)

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
        option_layout.setStretch(1, 2)
        option_layout.addWidget(self.sample_size_label)
        option_layout.addWidget(self.sample_size_edit)
        option_layout.setStretch(3, 1)
        option_layout.addWidget(self.sample_type_label)
        option_layout.addWidget(self.sample_type_combo)
        option_layout.setStretch(5, 1)
        option_layout.addWidget(self.log_base_label)
        option_layout.addWidget(self.log_base_edit)
        option_layout.setStretch(7, 1)

        layout = QtWidgets.QVBoxLayout()
        layout.addLayout(option_layout)
        layout.addWidget(self.view)

        self.setLayout(layout)

    def reset(self) -> None:
        self.view.reset()

    @QtCore.Slot(str, float)
    def _on_input_range_changed(self, label: str, value: float) -> None:
        self.view.set_input_range(
            self.input_range_edit.component_value("min"),
            self.input_range_edit.component_value("max"),
        )

    @QtCore.Slot(int)
    def _on_sample_size_changed(self, sample_size: int) -> None:
        if sample_size >= CurveView.SAMPLE_SIZE_MIN:
            self.view.set_sample_size(sample_size)
        else:
            with SignalsBlocked(self.sample_size_edit):
                self.sample_size_edit.set_value(CurveView.SAMPLE_SIZE_MIN)
                self.view.set_sample_size(CurveView.SAMPLE_SIZE_MIN)

    @QtCore.Slot(int)
    def _on_sample_type_changed(self, index: int) -> None:
        sample_type = self.sample_type_combo.member()
        self.log_base_edit.setEnabled(sample_type == SampleType.LOG)
        self.view.set_sample_type(sample_type)

    @QtCore.Slot(int)
    def _on_log_base_changed(self, log_base: int) -> None:
        self.view.set_log_base(log_base)


class CurveView(QtWidgets.QGraphicsView):

    SAMPLE_SIZE_MIN = 2**8
    SAMPLE_SIZE_DEFAULT = 2**10
    INPUT_MIN_DEFAULT = 0.0
    INPUT_MAX_DEFAULT = 1.0
    CURVE_SCALE = 100
    FONT_HEIGHT = 4

    # The curve viewer only shows 5 digit decimal precision, so this should work fine
    # as a minimum when input min is 0.
    LOG_EPSILON = 1e-5
    LOG_BASE_DEFAULT = 2

    def __init__(
        self,
        input_min: float = INPUT_MIN_DEFAULT,
        input_max: float = INPUT_MAX_DEFAULT,
        sample_size: int = SAMPLE_SIZE_DEFAULT,
        sample_type: SampleType = SampleType.LINEAR,
        log_base: int = LOG_BASE_DEFAULT,
        parent: Optional[QtWidgets.QWidget] = None,
    ):
        super().__init__(parent=parent)
        self.setRenderHint(QtGui.QPainter.Antialiasing, True)
        self.setViewportUpdateMode(QtWidgets.QGraphicsView.FullViewportUpdate)
        self.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setMouseTracking(True)
        self.scale(1, -1)

        if input_max <= input_min:
            input_min = self.INPUT_MIN_DEFAULT
            input_max = self.INPUT_MAX_DEFAULT

        self._log_base = max(2, log_base)
        self._input_min = input_min
        self._input_max = input_max
        self._sample_size = max(self.SAMPLE_SIZE_MIN, sample_size)
        self._sample_type = sample_type
        self._sample_ellipse: Optional[QtWidgets.QGraphicsEllipseItem] = None
        self._sample_text: Optional[QtWidgets.QGraphicsTextItem] = None
        self._sample_rect: Optional[QtCore.QRectF] = None
        self._samples_x_lin: np.ndarray = None
        self._samples_x_log: np.ndarray = None
        self._sample_y_min: float = self._input_min
        self._sample_y_max: float = self._input_max
        self._samples: dict[str, np.ndarray] = {}
        self._curve_paths: dict[str, QtGui.QPainterPath] = {}
        self._curve_items: dict[str, QtWidgets.QGraphicsPathItem] = {}
        self._curve_rect = QtCore.QRectF(
            self._input_min,
            self._input_min,
            self._input_max - self._input_min,
            self._input_max - self._input_min,
        )
        self._nearest_samples: dict[str, tuple[float, float, float]] = {}
        self._prev_cpu_proc = None
        self._curve_init = False

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
        super().resizeEvent(event)

        self._fit()

    def mouseMoveEvent(self, event: QtGui.QMouseEvent) -> None:
        """Find nearest samples to current mouse position."""
        super().mouseMoveEvent(event)

        if not self._curve_init:
            return

        self._nearest_samples.clear()

        pos = self.mapToScene(event.pos()) / self.CURVE_SCALE
        pos_arr = np.array([pos.x(), pos.y()], dtype=np.float32)

        for color_name, samples in self._samples.items():
            all_dist = np.linalg.norm(samples - pos_arr, axis=2)
            nearest_dist_index = np.argmin(all_dist)
            self._nearest_samples[color_name] = (
                (
                    self._samples_x_lin[nearest_dist_index]
                    if self._sample_type == SampleType.LINEAR
                    else self._samples_x_log[nearest_dist_index]
                ),
                samples[0][nearest_dist_index][0],
                samples[0][nearest_dist_index][1],
            )

        self._invalidate()

    def reset(self) -> None:
        """Clear all curves."""
        self._samples.clear()
        self._curve_items.clear()
        self._curve_paths.clear()
        self._nearest_samples.clear()
        self._prev_cpu_proc = None
        self._curve_init = False

        self._scene.clear()
        self._invalidate()

    def set_input_range(self, input_min: float, input_max: float) -> None:
        if (
            input_min != self._input_min or input_max != self._input_max
        ) and input_max > input_min:
            self._input_min = input_min
            self._input_max = input_max
            self._update_curves()

    def set_input_min(self, input_min: float) -> None:
        if input_min != self._input_min and input_min < self._input_max:
            self._input_min = input_min
            self._update_curves()

    def set_input_max(self, input_max: float) -> None:
        if input_max != self._input_max and input_max > self._input_min:
            self._input_max = input_max
            self._update_curves()

    def set_sample_size(self, sample_size: int) -> None:
        if sample_size != self._sample_size and sample_size >= self.SAMPLE_SIZE_MIN:
            self._sample_size = sample_size
            self._update_curves()

    def set_sample_type(self, sample_type: SampleType) -> None:
        if sample_type != self._sample_type:
            self._sample_type = sample_type
            self._update_curves()

    def set_log_base(self, log_base: int) -> None:
        if log_base != self._log_base and log_base >= 2:
            self._log_base = log_base
            self._update_curves()

    def drawBackground(self, painter: QtGui.QPainter, rect: QtCore.QRectF) -> None:
        # Flood fill background
        painter.setPen(QtCore.Qt.NoPen)
        painter.setBrush(QtCore.Qt.black)
        painter.drawRect(rect)

        if not self._curve_init:
            return

        # Calculate grid rect
        curve_t, curve_b, curve_l, curve_r = (
            self._curve_rect.top(),
            self._curve_rect.bottom(),
            self._curve_rect.left(),
            self._curve_rect.right(),
        )

        # Round to nearest 10 for grid bounds
        grid_t = curve_t // 10 * 10
        if grid_t > math.ceil(curve_t):
            grid_t -= 10
        grid_b = curve_b // 10 * 10
        if grid_b < math.floor(curve_b):
            grid_b += 10
        grid_l = curve_l // 10 * 10
        if grid_l > math.ceil(curve_l):
            grid_l -= 10
        grid_r = curve_r // 10 * 10
        if grid_r < math.floor(curve_r):
            grid_r += 10

        text_pen = QtGui.QPen(GRAY_COLOR)

        grid_pen = QtGui.QPen(GRAY_COLOR.darker(200))
        grid_pen.setWidthF(0)

        font = painter.font()
        font.setPixelSize(self.FONT_HEIGHT)

        painter.setBrush(QtCore.Qt.NoBrush)
        painter.setFont(font)

        # Calculate samples to display
        sample_step = math.ceil(self._sample_size / 10.0)
        min_x = max_x = min_y = max_y = None

        if self._sample_type == SampleType.LINEAR:
            sample_x_values = self._samples_x_lin
        else:  # SampleType.LOG
            sample_x_values = self._samples_x_log

        sample_y_data = []
        for i, sample_y in enumerate(
            np.linspace(self._sample_y_min, self._sample_y_max, 11, dtype=np.float32)
        ):
            pos_y = sample_y * self.CURVE_SCALE
            sample_y_data.append((pos_y, sample_y))

            if min_y is None or pos_y < min_y:
                min_y = pos_y
            if max_y is None or pos_y > max_y:
                max_y = pos_y

        sample_x_data = []
        for i, sample_x in enumerate(sample_x_values):
            if not (i % sample_step == 0 or i == self._sample_size - 1):
                continue

            pos_x = self._samples_x_lin[i] * self.CURVE_SCALE
            sample_x_data.append((pos_x, sample_x))

            if min_x is None or pos_x < min_x:
                min_x = pos_x
            if max_x is None or pos_x > max_x:
                max_x = pos_x

        if min_x is None:
            min_x = grid_l
        if max_x is None:
            max_x = grid_r
        if min_y is None:
            min_y = grid_t
        if max_x is None:
            max_y = grid_b

        self._sample_rect = QtCore.QRectF(
            max_x + 5, min_y, 40, len(self._curve_items) * 20
        )

        # Draw grid rows
        y_text_origin = QtGui.QTextOption(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
        y_text_origin.setWrapMode(QtGui.QTextOption.NoWrap)

        for pos_y, sample_y in sample_y_data:
            painter.setPen(grid_pen)
            painter.drawLine(QtCore.QLineF(min_x, pos_y, max_x, pos_y))

            if pos_y > grid_t:
                label_value = round(sample_y, 2)
                if label_value == 0.0:
                    label_value = abs(label_value)

                painter.save()
                painter.translate(QtCore.QPointF(min_x, pos_y))
                painter.scale(1, -1)
                painter.setPen(text_pen)
                painter.drawText(
                    QtCore.QRectF(-42.5, -10, 40, 20), str(label_value), y_text_origin
                )
                painter.restore()

        # Draw grid columns
        x_text_origin = QtGui.QTextOption(QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter)
        x_text_origin.setWrapMode(QtGui.QTextOption.NoWrap)

        for pos_x, sample_x in sample_x_data:
            painter.setPen(grid_pen)
            painter.drawLine(QtCore.QLineF(pos_x, min_y, pos_x, max_y))

            if pos_x > grid_l:
                label_value = round(
                    sample_x, 5 if self._sample_type == SampleType.LOG else 2
                )
                if label_value == 0.0:
                    label_value = abs(label_value)

                painter.save()
                painter.translate(QtCore.QPointF(pos_x, min_y))
                painter.scale(1, -1)
                painter.rotate(90)
                painter.setPen(text_pen)
                painter.drawText(
                    QtCore.QRect(2.5 + 1, -10, 40, 20), str(label_value), x_text_origin
                )
                painter.restore()

    def drawForeground(self, painter: QtGui.QPainter, rect: QtCore.QRectF) -> None:
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
                QtCore.QPointF(
                    nearest_sample[1] * self.CURVE_SCALE,
                    nearest_sample[2] * self.CURVE_SCALE,
                ),
                1.25,
                1.25,
            )

            if sample_l is not None:
                # Draw sample values
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
                    painter.setPen(palette.color(palette.Text))
                else:
                    painter.setPen(QtGui.QColor(color_name))
                painter.drawText(
                    QtCore.QRectF(5, -20, 35, 10), x_label_value, text_origin
                )
                painter.drawText(
                    QtCore.QRectF(5, -10, 35, 10), y_label_value, text_origin
                )

                painter.restore()

    def _update_curves(self) -> None:
        self._update_x_samples()
        if self._prev_cpu_proc is not None:
            self._on_cpu_processor_ready(self._prev_cpu_proc)

    def _fit(self) -> None:
        if not self._curve_init:
            return

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

        self.fitInView(fit_rect, QtCore.Qt.KeepAspectRatio)
        self.centerOn(fit_rect.center())
        self.update()

    def _invalidate(self) -> None:
        self._scene.invalidate(QtCore.QRectF(self.visibleRegion().boundingRect()))

    def _update_x_samples(self):
        self._samples_x_lin = np.linspace(
            self._input_min, self._input_max, self._sample_size, dtype=np.float32
        )
        self._samples_x_log = np.logspace(
            math.log(max(self.LOG_EPSILON, self._input_min)),
            math.log(max(self.LOG_EPSILON, self._input_max)),
            self._sample_size,
            base=self._log_base,
            dtype=np.float32,
        )

    @QtCore.Slot(ocio.CPUProcessor)
    def _on_cpu_processor_ready(self, cpu_proc: ocio.CPUProcessor) -> None:
        """
        Update curves from OCIO CPU processor.

        :param cpu_proc: CPU processor of currently viewed transform
        """
        self.reset()

        self._prev_cpu_proc = cpu_proc

        # Get input samples
        if self._sample_type == SampleType.LOG:
            samples = self._samples_x_log
        else:  # LINEAR
            samples = self._samples_x_lin

        # Interleave samples per channel
        rgb_samples = np.repeat(samples, 3)

        # Apply processor to samples
        cpu_proc.applyRGB(rgb_samples)

        # De-interleave transformed samples
        r_samples = rgb_samples[0::3]
        g_samples = rgb_samples[1::3]
        b_samples = rgb_samples[2::3]

        # Build painter path from sample data
        if np.array_equal(r_samples, g_samples) and np.array_equal(
            r_samples, b_samples
        ):
            palette = self.palette()
            color_name = palette.color(palette.Text).name()

            self._samples[color_name] = np.dstack((self._samples_x_lin, r_samples))
            self._sample_y_min = r_samples.min()
            self._sample_y_max = r_samples.max()

            curve = QtGui.QPainterPath(
                QtCore.QPointF(self._samples_x_lin[0], r_samples[0])
            )
            curve.reserve(samples.size)
            for i in range(1, samples.size):
                curve.lineTo(QtCore.QPointF(self._samples_x_lin[i], r_samples[i]))
            self._curve_paths[color_name] = curve

        else:
            sample_y_min = None
            sample_y_max = None

            for i, (color, channel_samples) in enumerate(
                [
                    (R_COLOR, r_samples),
                    (G_COLOR, g_samples),
                    (B_COLOR, b_samples),
                ]
            ):
                color_name = color.name()

                self._samples[color_name] = np.dstack(
                    (self._samples_x_lin, channel_samples)
                )

                channel_sample_y_min = channel_samples.min()
                if sample_y_min is None or channel_sample_y_min < sample_y_min:
                    sample_y_min = channel_sample_y_min
                channel_sample_y_max = channel_samples.max()
                if sample_y_max is None or channel_sample_y_max > sample_y_max:
                    sample_y_max = channel_sample_y_max

                curve = QtGui.QPainterPath(
                    QtCore.QPointF(self._samples_x_lin[0], channel_samples[0])
                )
                curve.reserve(samples.size)
                for j in range(1, samples.size):
                    curve.lineTo(
                        QtCore.QPointF(self._samples_x_lin[j], channel_samples[j])
                    )
                self._curve_paths[color_name] = curve

            self._sample_y_min = sample_y_min
            self._sample_y_max = sample_y_max

        # Add curve(s) to scene
        self._curve_rect = QtCore.QRectF()
        for color_name, curve in self._curve_paths.items():
            pen = QtGui.QPen()
            pen.setColor(QtGui.QColor(color_name))
            pen.setWidthF(0)

            curve_item = self._scene.addPath(
                curve, pen, QtGui.QBrush(QtCore.Qt.NoBrush)
            )
            curve_item.setScale(self.CURVE_SCALE)
            self._curve_items[color_name] = curve_item
            self._curve_rect = self._curve_rect.united(curve_item.sceneBoundingRect())

        self._curve_init = True

        # Expand scene rect to fit graph
        max_dim = max(self._curve_rect.width(), self._curve_rect.height()) * 2
        scene_rect = self._curve_rect.adjusted(-max_dim, -max_dim, max_dim, max_dim)
        self.setSceneRect(scene_rect)

        self._fit()
