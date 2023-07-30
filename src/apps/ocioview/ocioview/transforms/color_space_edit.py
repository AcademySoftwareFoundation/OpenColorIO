# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..config_cache import ConfigCache
from ..widgets import CheckBox, CallbackComboBox
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class ColorSpaceTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "ph.swap"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Widget
        self.src_combo = CallbackComboBox(ConfigCache.get_color_space_names)
        self.src_combo.currentIndexChanged.connect(self._on_edit)

        self.dst_combo = CallbackComboBox(ConfigCache.get_color_space_names)
        self.dst_combo.currentIndexChanged.connect(self._on_edit)

        self.data_bypass_check = CheckBox("Data Bypass")
        self.data_bypass_check.stateChanged.connect(self._on_edit)

        # Layout
        self.tf_layout.insertRow(0, "", self.data_bypass_check)
        self.tf_layout.insertRow(0, "Destination", self.dst_combo)
        self.tf_layout.insertRow(0, "Source", self.src_combo)

        # Initialize
        self.update_from_config()

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = super().transform()
        transform.setSrc(self.src_combo.currentText())
        transform.setDst(self.dst_combo.currentText())
        transform.setDataBypass(self.data_bypass_check.isChecked())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.src_combo.setCurrentText(transform.getSrc())
        self.dst_combo.setCurrentText(transform.getDst())
        self.data_bypass_check.setChecked(transform.getDataBypass())

    def update_from_config(self):
        """
        Update available color spaces from current config.
        """
        self.src_combo.update()
        self.dst_combo.update()


TransformEditFactory.register(ocio.ColorSpaceTransform, ColorSpaceTransformEdit)
