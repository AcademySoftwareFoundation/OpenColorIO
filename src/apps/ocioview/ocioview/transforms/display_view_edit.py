# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtWidgets

from ..config_cache import ConfigCache
from ..utils import SignalsBlocked
from ..widgets import CheckBox, ComboBox, CallbackComboBox
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class DisplayViewTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "mdi6.monitor-eye"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Widget
        self.src_combo = CallbackComboBox(ConfigCache.get_color_space_names)
        self.src_combo.currentIndexChanged.connect(self._on_edit)

        self.display_combo = CallbackComboBox(
            ConfigCache.get_displays,
            get_default_item=lambda: ocio.GetCurrentConfig().getDefaultDisplay(),
        )
        self.display_combo.currentIndexChanged.connect(self._on_display_changed)
        self.display_combo.currentIndexChanged.connect(self._on_edit)

        self.view_combo = ComboBox()
        self.view_combo.currentIndexChanged.connect(self._on_edit)

        self.looks_bypass_check = CheckBox("Looks Bypass")
        self.looks_bypass_check.stateChanged.connect(self._on_edit)

        self.data_bypass_check = CheckBox("Data Bypass")
        self.data_bypass_check.stateChanged.connect(self._on_edit)

        # Layout
        bypass_layout = QtWidgets.QHBoxLayout()
        bypass_layout.addWidget(self.looks_bypass_check)
        bypass_layout.addWidget(self.data_bypass_check)
        bypass_layout.addStretch()

        self.tf_layout.insertRow(0, "", bypass_layout)
        self.tf_layout.insertRow(0, "View", self.view_combo)
        self.tf_layout.insertRow(0, "Display", self.display_combo)
        self.tf_layout.insertRow(0, "Source", self.src_combo)

        # Initialize
        self.update_from_config()

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = super().transform()
        transform.setSrc(self.src_combo.currentText())
        transform.setDisplay(self.display_combo.currentText())
        transform.setView(self.view_combo.currentText())
        transform.setLooksBypass(self.looks_bypass_check.isChecked())
        transform.setDataBypass(self.data_bypass_check.isChecked())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.src_combo.setCurrentText(transform.getSrc())
        self.display_combo.setCurrentText(transform.getDisplay())
        self.view_combo.setCurrentText(transform.getView())
        self.looks_bypass_check.setChecked(transform.getLooksBypass())
        self.data_bypass_check.setChecked(transform.getDataBypass())

    def update_from_config(self):
        """
        Update available color spaces and displays from the current
        config.
        """
        self.src_combo.update()
        self.display_combo.update()
        self._on_display_changed(self.display_combo.currentIndex())

    @QtCore.Slot(int)
    def _on_display_changed(self, index: int):
        """
        Update available views for the selected display from the
        current config.
        """
        config = ocio.GetCurrentConfig()
        display = self.display_combo.itemText(index)
        view = self.view_combo.currentText()
        color_space_name = self.src_combo.currentText()

        with SignalsBlocked(self.view_combo):
            self.view_combo.clear()
            self.view_combo.addItems(ConfigCache.get_views(display, color_space_name))

        view_index = self.view_combo.findText(view)
        if view_index != -1:
            self.view_combo.setCurrentIndex(view_index)
        else:
            self.view_combo.setCurrentText(
                config.getDefaultView(display, color_space_name)
            )


TransformEditFactory.register(ocio.DisplayViewTransform, DisplayViewTransformEdit)
