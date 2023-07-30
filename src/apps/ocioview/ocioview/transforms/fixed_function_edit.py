# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtWidgets

from ..widgets import EnumComboBox, FloatEditArray, ExpandingStackedWidget
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class FixedFunctionTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "mdi6.function-variant"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Widgets
        self.style_combo = EnumComboBox(ocio.FixedFunctionStyle)
        self.style_combo.currentIndexChanged.connect(self._on_style_changed)
        self.style_combo.currentIndexChanged.connect(self._on_edit)

        self.no_params = QtWidgets.QLabel("<i>no params</i>")
        self.no_params.setDisabled(True)

        self.rec2100_surround_edit = FloatEditArray(("gamma",))
        self.rec2100_surround_edit.value_changed.connect(self._on_edit)

        self.aces_gamut_comp_13_edit = FloatEditArray(
            ("lim c", "lim m", "lim y", "thr c", "thr m", "thr y", "power"),
            shape=(3, 3),
        )
        self.aces_gamut_comp_13_edit.value_changed.connect(self._on_edit)

        self.param_widgets = {
            ocio.FIXED_FUNCTION_REC2100_SURROUND: self.rec2100_surround_edit,
            ocio.FIXED_FUNCTION_ACES_GAMUT_COMP_13: self.aces_gamut_comp_13_edit,
        }

        self.param_stack = ExpandingStackedWidget()
        self.param_stack.addWidget(self.no_params)
        self.param_stack.addWidget(self.rec2100_surround_edit)
        self.param_stack.addWidget(self.aces_gamut_comp_13_edit)

        # Layout
        self.tf_layout.insertRow(0, "Params", self.param_stack)
        self.tf_layout.insertRow(0, "Style", self.style_combo)

        # Initialize
        self._on_style_changed(self.style_combo.currentIndex())

    def transform(self) -> ocio.ColorSpaceTransform:
        style = self.style_combo.member()
        param_widget = self.param_widgets.get(style)
        params = []
        if param_widget is not None:
            params = param_widget.value()

        transform = self.__tf_type__(style)
        transform.setParams(params)
        transform.setDirection(self.direction_combo.member())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)

        style = transform.getStyle()
        param_widget = self.param_widgets.get(style)
        if param_widget is not None:
            param_widget.set_value(transform.getParams())

        self.style_combo.set_member(style)

    @QtCore.Slot(int)
    def _on_style_changed(self, index: int):
        """
        Toggle style-specific parameter widgets in the parameter stack.
        """
        style = self.style_combo.member()
        widget = self.param_widgets.get(style, self.no_params)
        self.param_stack.setCurrentWidget(widget)


TransformEditFactory.register(ocio.FixedFunctionTransform, FixedFunctionTransformEdit)
