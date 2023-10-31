# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..constants import RGB
from ..widgets import FloatEditArray
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class LogAffineTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "mdi6.chart-bell-curve-cumulative"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        transform = self.create_transform()

        # Widgets
        self.log_side_slope_edit = FloatEditArray(RGB, transform.getLogSideSlopeValue())
        self.log_side_slope_edit.value_changed.connect(self._on_edit)

        self.log_side_offset_edit = FloatEditArray(
            RGB, transform.getLogSideOffsetValue()
        )
        self.log_side_offset_edit.value_changed.connect(self._on_edit)

        self.lin_side_slope_edit = FloatEditArray(RGB, transform.getLinSideSlopeValue())
        self.lin_side_slope_edit.value_changed.connect(self._on_edit)

        self.lin_side_offset_edit = FloatEditArray(
            RGB, transform.getLinSideOffsetValue()
        )
        self.lin_side_offset_edit.value_changed.connect(self._on_edit)

        # Layout
        self.tf_layout.insertRow(0, "Lin Side Offset", self.lin_side_offset_edit)
        self.tf_layout.insertRow(0, "Lin Side Slope", self.lin_side_slope_edit)
        self.tf_layout.insertRow(0, "Log Side Offset", self.log_side_offset_edit)
        self.tf_layout.insertRow(0, "Log Side Slope", self.log_side_slope_edit)

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = super().transform()
        transform.setLogSideSlopeValue(self.log_side_slope_edit.value())
        transform.setLogSideOffsetValue(self.log_side_offset_edit.value())
        transform.setLinSideSlopeValue(self.lin_side_slope_edit.value())
        transform.setLinSideOffsetValue(self.lin_side_offset_edit.value())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.log_side_slope_edit.set_value(transform.getLogSideSlopeValue())
        self.log_side_offset_edit.set_value(transform.getLogSideOffsetValue())
        self.lin_side_slope_edit.set_value(transform.getLinSideSlopeValue())
        self.lin_side_offset_edit.set_value(transform.getLinSideOffsetValue())


TransformEditFactory.register(ocio.LogAffineTransform, LogAffineTransformEdit)
