# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..config_cache import ConfigCache
from ..widgets import CheckBox, CallbackComboBox, LineEdit
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class LookTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "ph.aperture"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Widgets
        self.src_combo = CallbackComboBox(ConfigCache.get_color_space_names)
        self.src_combo.currentIndexChanged.connect(self._on_edit)

        self.dst_combo = CallbackComboBox(ConfigCache.get_color_space_names)
        self.dst_combo.currentIndexChanged.connect(self._on_edit)

        self.skip_color_space_conversion_check = CheckBox("Skip Color Space Conversion")
        self.skip_color_space_conversion_check.stateChanged.connect(self._on_edit)

        # TODO: Add look completer and validator
        self.looks_edit = LineEdit()
        self.looks_edit.editingFinished.connect(self._on_edit)

        # Layout
        self.tf_layout.insertRow(0, "", self.skip_color_space_conversion_check)
        self.tf_layout.insertRow(0, "Looks", self.looks_edit)
        self.tf_layout.insertRow(0, "Destination", self.dst_combo)
        self.tf_layout.insertRow(0, "Source", self.src_combo)

        # initialize
        self.update_from_config()

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = super().transform()
        transform.setSrc(self.src_combo.currentText())
        transform.setDst(self.dst_combo.currentText())
        transform.setLooks(self.looks_edit.text())
        transform.setSkipColorSpaceConversion(
            self.skip_color_space_conversion_check.isChecked()
        )
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.src_combo.setCurrentText(transform.getSrc())
        self.dst_combo.setCurrentText(transform.getDst())
        self.looks_edit.setText(transform.getLooks())
        self.skip_color_space_conversion_check.setChecked(
            transform.getSkipColorSpaceConversion()
        )

    def update_from_config(self):
        """
        Update available color spaces from current config.
        """
        self.src_combo.update()
        self.dst_combo.update()


TransformEditFactory.register(ocio.LookTransform, LookTransformEdit)
