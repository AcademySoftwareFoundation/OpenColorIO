# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import RGB, RGBA
from ..widgets import (
    ComboBox,
    FloatEdit,
    FloatEditArray,
    IntEditArray,
    FormLayout,
    ExpandingStackedWidget,
)
from ..utils import m44_to_m33, m33_to_m44, v4_to_v3, v3_to_v4
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class MatrixTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "mdi6.matrix"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        config = ocio.GetCurrentConfig()
        transform = self.__tf_type__.Identity()

        # Widgets
        # Matrix
        self.matrix_edit = FloatEditArray(
            tuple([""] * 12), m44_to_m33(transform.getMatrix()), shape=(3, 3)
        )
        self.matrix_edit.value_changed.connect(self._on_edit)

        self.offset_edit = FloatEditArray(
            tuple([""] * 3), v4_to_v3(transform.getOffset())
        )
        self.offset_edit.value_changed.connect(self._on_edit)

        matrix_layout = FormLayout()
        matrix_layout.addRow("Matrix", self.matrix_edit)
        matrix_layout.addRow("Offset", self.offset_edit)
        self.matrix_params = QtWidgets.QFrame()
        self.matrix_params.setObjectName("matrix_transform_edit_matrix_params")
        self.matrix_params.setLayout(matrix_layout)
        self.matrix_params.setStyleSheet(
            f"QFrame#matrix_transform_edit_matrix_params {{"
            f"    border-top: 1px solid "
            f"    {self.palette().color(QtGui.QPalette.Base).name()};"
            f"    padding-top: 8px;"
            f"}}"
        )

        no_params = QtWidgets.QFrame()
        no_params.setMaximumHeight(0)

        # Sat
        self.sat_edit = FloatEdit(1.0)
        self.sat_edit.value_changed.connect(self._on_edit)

        self.sat_luma_coef_edit = FloatEditArray(RGB, config.getDefaultLumaCoefs())
        self.sat_luma_coef_edit.value_changed.connect(self._on_edit)

        sat_layout = FormLayout()
        sat_layout.addRow("Saturation", self.sat_edit)
        sat_layout.addRow("Luma Coefficients", self.sat_luma_coef_edit)
        self.sat_params = QtWidgets.QFrame()
        self.sat_params.setLayout(sat_layout)

        # Scale
        self.scale_edit = FloatEditArray(RGB, (1.0, 1.0, 1.0))
        self.scale_edit.value_changed.connect(self._on_edit)

        scale_layout = FormLayout()
        scale_layout.addRow("Scale", self.scale_edit)
        self.scale_params = QtWidgets.QFrame()
        self.scale_params.setLayout(scale_layout)

        # View
        self.channel_hot_edit = IntEditArray(RGBA, (1, 1, 1, 1))
        self.channel_hot_edit.value_changed.connect(self._on_edit)

        self.view_luma_coef_edit = FloatEditArray(RGB, config.getDefaultLumaCoefs())
        self.view_luma_coef_edit.value_changed.connect(self._on_edit)

        view_layout = FormLayout()
        view_layout.addRow("Channel Hot", self.channel_hot_edit)
        view_layout.addRow("Luma Coefficients", self.view_luma_coef_edit)
        self.view_params = QtWidgets.QFrame()
        self.view_params.setLayout(view_layout)

        self.params_stack = ExpandingStackedWidget()
        self.params_stack.addWidget(no_params)
        self.params_stack.addWidget(self.sat_params)
        self.params_stack.addWidget(self.scale_params)
        self.params_stack.addWidget(self.view_params)

        self.params_combo = ComboBox()
        self.params_combo.addItems(["Matrix", "Saturation", "Scale", "View"])
        self.params_combo.currentIndexChanged.connect(self._on_edit)

        # Link parameter selection to parameter stack current index
        self.params_combo.currentIndexChanged[int].connect(
            self.params_stack.setCurrentIndex
        )

        # Layout
        self.tf_layout.insertRow(0, self.matrix_params)
        self.tf_layout.insertRow(0, self.params_stack)
        self.tf_layout.insertRow(0, self.params_combo)

    def transform(self) -> ocio.ColorSpaceTransform:
        params_choice = self.params_combo.currentText()

        if params_choice == "Saturation":
            transform = self.__tf_type__.Sat(
                self.sat_edit.value(), self.sat_luma_coef_edit.value()
            )
        elif params_choice == "Scale":
            transform = self.__tf_type__.Scale(v3_to_v4(self.scale_edit.value()))
        elif params_choice == "View":
            transform = self.__tf_type__.View(
                self.channel_hot_edit.value(), self.view_luma_coef_edit.value()
            )
        else:  # Matrix
            transform = self.create_transform()
            transform.setMatrix(m33_to_m44(self.matrix_edit.value()))
            transform.setOffset(v3_to_v4(self.offset_edit.value()))

        transform.setDirection(self.direction_combo.member())

        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.matrix_edit.set_value(m44_to_m33(transform.getMatrix()))
        self.offset_edit.set_value(v4_to_v3(transform.getOffset()))


TransformEditFactory.register(ocio.MatrixTransform, MatrixTransformEdit)
