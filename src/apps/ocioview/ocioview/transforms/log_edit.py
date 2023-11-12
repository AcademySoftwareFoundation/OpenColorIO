# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..widgets import FloatEdit
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class LogTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "mdi6.math-log"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        transform = self.create_transform()

        # Widgets
        self.base_edit = FloatEdit(transform.getBase())
        self.base_edit.value_changed.connect(self._on_edit)

        # Layout
        self.tf_layout.insertRow(0, "Base", self.base_edit)

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = super().transform()
        transform.setBase(self.base_edit.value())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.base_edit.set_value(transform.getBase())


TransformEditFactory.register(ocio.LogTransform, LogTransformEdit)
