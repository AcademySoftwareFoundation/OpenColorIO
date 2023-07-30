# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..widgets import EnumComboBox, FloatEdit
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class ExposureContrastTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "ph.sliders-horizontal"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        transform = self.create_transform()

        # Widgets
        self.style_combo = EnumComboBox(ocio.ExposureContrastStyle)
        self.style_combo.set_member(transform.getStyle())
        self.style_combo.currentIndexChanged.connect(self._on_edit)

        self.exposure_edit = FloatEdit(transform.getExposure())
        self.exposure_edit.value_changed.connect(self._on_edit)

        self.contrast_edit = FloatEdit(transform.getContrast())
        self.contrast_edit.value_changed.connect(self._on_edit)

        self.gamma_edit = FloatEdit(transform.getGamma())
        self.gamma_edit.value_changed.connect(self._on_edit)

        self.pivot_edit = FloatEdit(transform.getPivot())
        self.pivot_edit.value_changed.connect(self._on_edit)

        self.log_exposure_step_edit = FloatEdit(transform.getLogExposureStep())
        self.log_exposure_step_edit.value_changed.connect(self._on_edit)

        self.log_mid_gray_edit = FloatEdit(transform.getLogMidGray())
        self.log_mid_gray_edit.value_changed.connect(self._on_edit)

        # Layout
        self.tf_layout.insertRow(0, "Log Mid Gray", self.log_mid_gray_edit)
        self.tf_layout.insertRow(0, "Log Exposure Step", self.log_exposure_step_edit)
        self.tf_layout.insertRow(0, "Pivot", self.pivot_edit)
        self.tf_layout.insertRow(0, "Gamma", self.gamma_edit)
        self.tf_layout.insertRow(0, "Contrast", self.contrast_edit)
        self.tf_layout.insertRow(0, "Exposure", self.exposure_edit)
        self.tf_layout.insertRow(0, "Style", self.style_combo)

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = super().transform()
        transform.setStyle(self.style_combo.member())
        transform.setExposure(self.exposure_edit.value())
        transform.setContrast(self.contrast_edit.value())
        transform.setGamma(self.gamma_edit.value())
        transform.setPivot(self.pivot_edit.value())
        transform.setLogExposureStep(self.log_exposure_step_edit.value())
        transform.setLogMidGray(self.log_mid_gray_edit.value())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.style_combo.set_member(transform.getStyle())
        self.exposure_edit.set_value(transform.getExposure())
        self.contrast_edit.set_value(transform.getContrast())
        self.gamma_edit.set_value(transform.getGamma())
        self.pivot_edit.set_value(transform.getPivot())
        self.log_exposure_step_edit.set_value(transform.getLogExposureStep())
        self.log_mid_gray_edit.set_value(transform.getLogMidGray())


TransformEditFactory.register(
    ocio.ExposureContrastTransform, ExposureContrastTransformEdit
)
