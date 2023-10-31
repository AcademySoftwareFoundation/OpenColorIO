# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..widgets import EnumComboBox, FloatEdit, FloatEditArray
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class AllocationTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "mdi6.memory"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Widgets
        self.allocation_combo = EnumComboBox(ocio.Allocation)
        self.allocation_combo.currentIndexChanged.connect(self._on_allocation_changed)
        self.allocation_combo.currentIndexChanged.connect(self._on_edit)

        self.src_range_edit = FloatEditArray(("min", "max"), (0.0, 1.0))
        self.src_range_edit.value_changed.connect(self._on_edit)

        self.lin_offset_edit = FloatEdit(0.0)
        self.lin_offset_edit.value_changed.connect(self._on_edit)

        # Layout
        self.tf_layout.insertRow(0, "Linear Offset", self.lin_offset_edit)
        self.tf_layout.insertRow(0, "Source Range", self.src_range_edit)
        self.tf_layout.insertRow(0, "Allocation", self.allocation_combo)

        # Initialize
        self._on_allocation_changed(self.allocation_combo.currentIndex())

    def transform(self) -> ocio.ColorSpaceTransform:
        allocation = self.allocation_combo.member()
        vars_ = self.src_range_edit.value()
        if allocation == ocio.ALLOCATION_LG2:
            vars_.append(self.lin_offset_edit.value())

        transform = super().transform()
        transform.setAllocation(allocation)
        transform.setVars(vars_)
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.allocation_combo.set_member(transform.getAllocation())
        alloc_vars = transform.getVars() or [0.0, 1.0]
        if len(alloc_vars) >= 2:
            self.src_range_edit.set_value(alloc_vars[:2])
        if len(alloc_vars) > 2:
            self.lin_offset_edit.set_value(alloc_vars[2])

    @QtCore.Slot(int)
    def _on_allocation_changed(self, index: int):
        """
        Toggle enabled variable widgets for the selected allocation.
        """
        allocation = self.allocation_combo.member()
        self.src_range_edit.setEnabled(allocation != ocio.ALLOCATION_UNKNOWN)
        self.lin_offset_edit.setEnabled(allocation == ocio.ALLOCATION_LG2)


TransformEditFactory.register(ocio.AllocationTransform, AllocationTransformEdit)
