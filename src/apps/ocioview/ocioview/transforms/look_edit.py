# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..widgets import CheckBox, ColorSpaceComboBox, LineEdit
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class LookTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "ph.aperture"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Widgets
        self.src_combo = ColorSpaceComboBox(include_roles=True)
        self.src_combo.color_space_changed.connect(self._on_edit)

        self.dst_combo = ColorSpaceComboBox(include_roles=True)
        self.dst_combo.color_space_changed.connect(self._on_edit)

        self.skip_color_space_conversion_check = CheckBox(
            "Skip Color Space Conversion"
        )
        self.skip_color_space_conversion_check.stateChanged.connect(
            self._on_edit
        )

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
        transform.setSrc(self.src_combo.color_space_name())
        transform.setDst(self.dst_combo.color_space_name())
        transform.setLooks(self.looks_edit.text())
        transform.setSkipColorSpaceConversion(
            self.skip_color_space_conversion_check.isChecked()
        )
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.src_combo.set_color_space(transform.getSrc())
        self.dst_combo.set_color_space(transform.getDst())
        self.looks_edit.setText(transform.getLooks())
        self.skip_color_space_conversion_check.setChecked(
            transform.getSkipColorSpaceConversion()
        )

    def update_from_config(self):
        """
        Update available color spaces from current config.
        """
        self.src_combo.update_color_spaces()
        self.dst_combo.update_color_spaces()


TransformEditFactory.register(ocio.LookTransform, LookTransformEdit)
