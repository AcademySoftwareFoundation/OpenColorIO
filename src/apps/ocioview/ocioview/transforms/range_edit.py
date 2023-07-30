# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtWidgets

from ..widgets import CheckBox, EnumComboBox, FloatEdit
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class RangeTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "mdi6.code-brackets"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Widgets
        self.has_min_in = CheckBox("")
        self.has_min_in.stateChanged.connect(self._on_toggle_state_changed)
        self.has_min_in.stateChanged.connect(self._on_edit)

        self.min_in_edit = FloatEdit(0.0)
        self.min_in_edit.value_changed.connect(self._on_edit)

        self.has_max_in = CheckBox("")
        self.has_max_in.stateChanged.connect(self._on_toggle_state_changed)
        self.has_max_in.stateChanged.connect(self._on_edit)

        self.max_in_edit = FloatEdit(1.0)
        self.max_in_edit.value_changed.connect(self._on_edit)

        self.has_min_out = CheckBox("")
        self.has_min_out.stateChanged.connect(self._on_toggle_state_changed)
        self.has_min_out.stateChanged.connect(self._on_edit)

        self.min_out_edit = FloatEdit(0.0)
        self.min_out_edit.value_changed.connect(self._on_edit)

        self.has_max_out = CheckBox("")
        self.has_max_out.stateChanged.connect(self._on_toggle_state_changed)
        self.has_max_out.stateChanged.connect(self._on_edit)

        self.max_out_edit = FloatEdit(1.0)
        self.max_out_edit.value_changed.connect(self._on_edit)

        self.range_style_combo = EnumComboBox(ocio.RangeStyle)
        self.range_style_combo.currentIndexChanged.connect(self._on_edit)

        # Layout
        min_in_layout = QtWidgets.QHBoxLayout()
        min_in_layout.addWidget(self.has_min_in)
        min_in_layout.setStretchFactor(self.has_min_in, 0)
        min_in_layout.addWidget(self.min_in_edit)
        min_in_layout.setStretchFactor(self.min_in_edit, 1)

        max_in_layout = QtWidgets.QHBoxLayout()
        max_in_layout.addWidget(self.has_max_in)
        max_in_layout.setStretchFactor(self.has_max_in, 0)
        max_in_layout.addWidget(self.max_in_edit)
        max_in_layout.setStretchFactor(self.max_in_edit, 1)

        min_out_layout = QtWidgets.QHBoxLayout()
        min_out_layout.addWidget(self.has_min_out)
        min_out_layout.setStretchFactor(self.has_min_out, 0)
        min_out_layout.addWidget(self.min_out_edit)
        min_out_layout.setStretchFactor(self.min_out_edit, 1)

        max_out_layout = QtWidgets.QHBoxLayout()
        max_out_layout.addWidget(self.has_max_out)
        max_out_layout.setStretchFactor(self.has_max_out, 0)
        max_out_layout.addWidget(self.max_out_edit)
        max_out_layout.setStretchFactor(self.max_out_edit, 1)

        self.tf_layout.insertRow(0, "Range Style", self.range_style_combo)
        self.tf_layout.insertRow(0, "Max Out", max_out_layout)
        self.tf_layout.insertRow(0, "Min Out", min_out_layout)
        self.tf_layout.insertRow(0, "Max In", max_in_layout)
        self.tf_layout.insertRow(0, "Min In", min_in_layout)

        self._on_toggle_state_changed(0)

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = super().transform()
        if self.has_min_in.isChecked():
            transform.setMinInValue(self.min_in_edit.value())
        if self.has_max_in.isChecked():
            transform.setMaxInValue(self.max_in_edit.value())
        if self.has_min_out.isChecked():
            transform.setMinOutValue(self.min_out_edit.value())
        if self.has_max_out.isChecked():
            transform.setMaxOutValue(self.max_out_edit.value())
        transform.setStyle(self.range_style_combo.member())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)

        has_min_in = transform.hasMinInValue()
        self.has_min_in.setChecked(has_min_in)
        if has_min_in:
            self.min_in_edit.set_value(transform.getMinInValue())

        has_max_in = transform.hasMaxInValue()
        self.has_max_in.setChecked(has_max_in)
        if has_max_in:
            self.max_in_edit.set_value(transform.getMaxInValue())

        has_min_out = transform.hasMinOutValue()
        self.has_min_out.setChecked(has_min_out)
        if has_min_out:
            self.min_out_edit.set_value(transform.getMinOutValue())

        has_max_out = transform.hasMaxOutValue()
        self.has_max_out.setChecked(has_max_out)
        if has_max_out:
            self.max_out_edit.set_value(transform.getMaxOutValue())

        self.range_style_combo.set_member(transform.getStyle())

    @QtCore.Slot(int)
    def _on_toggle_state_changed(self, index: int):
        """
        Toggle range widget sections based on enabled bounds.
        """
        self.min_in_edit.setEnabled(self.has_min_in.isChecked())
        self.max_in_edit.setEnabled(self.has_max_in.isChecked())
        self.min_out_edit.setEnabled(self.has_min_out.isChecked())
        self.max_out_edit.setEnabled(self.has_max_out.isChecked())


TransformEditFactory.register(ocio.RangeTransform, RangeTransformEdit)
