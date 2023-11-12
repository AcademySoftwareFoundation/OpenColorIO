# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..constants import RGBA
from ..widgets import EnumComboBox, FloatEditArray
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class ExponentWithLinearTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "mdi6.exponent-box"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        transform = self.create_transform()

        # Widgets
        self.gamma_edit = FloatEditArray(RGBA, transform.getGamma())
        self.gamma_edit.value_changed.connect(self._on_edit)

        self.offset_edit = FloatEditArray(RGBA, transform.getOffset())
        self.offset_edit.value_changed.connect(self._on_edit)

        self.negative_style_combo = EnumComboBox(ocio.NegativeStyle)
        self.negative_style_combo.set_member(transform.getNegativeStyle())
        self.negative_style_combo.currentIndexChanged.connect(self._on_edit)

        # Layout
        self.tf_layout.insertRow(0, "Negative Style", self.negative_style_combo)
        self.tf_layout.insertRow(0, "Offset", self.offset_edit)
        self.tf_layout.insertRow(0, "Gamma", self.gamma_edit)

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = super().transform()
        transform.setGamma(self.gamma_edit.value())
        transform.setOffset(self.offset_edit.value())
        transform.setNegativeStyle(self.negative_style_combo.member())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.gamma_edit.set_value(transform.getGamma())
        self.offset_edit.set_value(transform.getOffset())
        self.negative_style_combo.set_member(transform.getNegativeStyle())


TransformEditFactory.register(
    ocio.ExponentWithLinearTransform, ExponentWithLinearTransformEdit
)
