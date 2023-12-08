# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..constants import RGB
from ..widgets import EnumComboBox, FloatEdit, FloatEditArray
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class CDLTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "ph.circles-three"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        transform = self.create_transform()

        # Widgets
        self.slope_edit = FloatEditArray(RGB, transform.getSlope())
        self.slope_edit.value_changed.connect(self._on_edit)

        self.offset_edit = FloatEditArray(RGB, transform.getOffset())
        self.offset_edit.value_changed.connect(self._on_edit)

        self.power_edit = FloatEditArray(RGB, transform.getPower())
        self.power_edit.value_changed.connect(self._on_edit)

        self.sat_edit = FloatEdit(transform.getSat())
        self.sat_edit.value_changed.connect(self._on_edit)

        self.style_combo = EnumComboBox(ocio.CDLStyle)
        self.style_combo.set_member(transform.getStyle())
        self.style_combo.currentIndexChanged.connect(self._on_edit)

        # Layout
        self.tf_layout.insertRow(0, "Style", self.style_combo)
        self.tf_layout.insertRow(0, "Saturation", self.sat_edit)
        self.tf_layout.insertRow(0, "Power", self.power_edit)
        self.tf_layout.insertRow(0, "Offset", self.offset_edit)
        self.tf_layout.insertRow(0, "Slope", self.slope_edit)

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = super().transform()
        transform.setSlope(self.slope_edit.value())
        transform.setOffset(self.offset_edit.value())
        transform.setPower(self.power_edit.value())
        transform.setSat(self.sat_edit.value())
        transform.setStyle(self.style_combo.member())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.slope_edit.set_value(transform.getSlope())
        self.offset_edit.set_value(transform.getOffset())
        self.power_edit.set_value(transform.getPower())
        self.sat_edit.set_value(transform.getSat())
        self.style_combo.set_member(transform.getStyle())


TransformEditFactory.register(ocio.CDLTransform, CDLTransformEdit)
