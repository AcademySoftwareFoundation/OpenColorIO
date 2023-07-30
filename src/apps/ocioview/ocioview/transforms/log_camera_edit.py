# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..constants import RGB
from ..widgets import FloatEdit, FloatEditArray
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class LogCameraTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "ph.video-camera"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        transform = self.__tf_type__([0.1, 0.1, 0.1])

        # Widgets
        self.base_edit = FloatEdit(transform.getBase())
        self.base_edit.value_changed.connect(self._on_edit)

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

        self.lin_side_break_edit = FloatEditArray(RGB, transform.getLinSideBreakValue())
        self.lin_side_break_edit.value_changed.connect(self._on_edit)

        # Unset linear slope is NaN
        self.linear_slope_edit = FloatEditArray(RGB, (0.0, 0.0, 0.0))
        self.linear_slope_edit.value_changed.connect(self._on_edit)

        # Layout
        self.tf_layout.insertRow(0, "Linear Slope", self.linear_slope_edit)
        self.tf_layout.insertRow(0, "Lin Side Break", self.lin_side_break_edit)
        self.tf_layout.insertRow(0, "Lin Side Offset", self.lin_side_offset_edit)
        self.tf_layout.insertRow(0, "Lin Side Slope", self.lin_side_slope_edit)
        self.tf_layout.insertRow(0, "Log Side Offset", self.log_side_offset_edit)
        self.tf_layout.insertRow(0, "Log Side Slope", self.log_side_slope_edit)
        self.tf_layout.insertRow(0, "Base", self.base_edit)

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = self.__tf_type__(self.lin_side_break_edit.value())
        transform.setBase(self.base_edit.value())
        transform.setLogSideSlopeValue(self.log_side_slope_edit.value())
        transform.setLogSideOffsetValue(self.log_side_offset_edit.value())
        transform.setLinSideSlopeValue(self.lin_side_slope_edit.value())
        transform.setLinSideOffsetValue(self.lin_side_offset_edit.value())
        transform.setLinSideBreakValue(self.lin_side_break_edit.value())
        transform.setLinearSlopeValue(self.linear_slope_edit.value())
        transform.setDirection(self.direction_combo.member())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.base_edit.set_value(transform.getBase())
        self.log_side_slope_edit.set_value(transform.getLogSideSlopeValue())
        self.log_side_offset_edit.set_value(transform.getLogSideOffsetValue())
        self.lin_side_slope_edit.set_value(transform.getLinSideSlopeValue())
        self.lin_side_offset_edit.set_value(transform.getLinSideOffsetValue())
        self.lin_side_break_edit.set_value(transform.getLinSideBreakValue())
        self.linear_slope_edit.set_value(transform.getLinearSlopeValue())


TransformEditFactory.register(ocio.LogCameraTransform, LogCameraTransformEdit)
