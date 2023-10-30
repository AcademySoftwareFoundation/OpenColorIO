# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..widgets import ComboBox
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class BuiltinTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "ph.package"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        transform = self.create_transform()

        # Widgets
        self.style_combo = ComboBox()

        registry = ocio.BuiltinTransformRegistry()
        tooltip_lines = []
        for style, desc in registry.getBuiltins():
            self.style_combo.addItem(style)
            tooltip_lines.append(f"{style}: {desc}")

        self.style_combo.setCurrentText(transform.getStyle())
        self.style_combo.setToolTip("\n".join(tooltip_lines))
        self.style_combo.currentIndexChanged.connect(self._on_edit)

        # Layout
        self.tf_layout.insertRow(0, "Style", self.style_combo)

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = super().transform()
        transform.setStyle(self.style_combo.currentText())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.style_combo.setCurrentText(transform.getStyle())


TransformEditFactory.register(ocio.BuiltinTransform, BuiltinTransformEdit)
