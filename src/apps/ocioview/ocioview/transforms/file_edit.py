# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import os
from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..widgets import EnumComboBox, LineEdit
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory


class FileTransformEdit(BaseTransformEdit):
    __icon_glyph__ = "mdi6.file-table-outline"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        transform = self.create_transform()

        # Widgets
        self.src_edit = LineEdit()
        self.src_edit.editingFinished.connect(self._on_src_changed)
        self.src_edit.editingFinished.connect(self._on_edit)

        self.ccc_id_edit = LineEdit()
        self.ccc_id_edit.setEnabled(False)
        self.ccc_id_edit.editingFinished.connect(self._on_edit)

        self.cdl_style_combo = EnumComboBox(ocio.CDLStyle)
        self.cdl_style_combo.set_member(transform.getCDLStyle())
        self.cdl_style_combo.setEnabled(False)
        self.cdl_style_combo.currentIndexChanged.connect(self._on_edit)

        self.interpolation_combo = EnumComboBox(ocio.Interpolation)
        self.interpolation_combo.set_member(transform.getInterpolation())
        self.interpolation_combo.currentIndexChanged.connect(self._on_edit)

        # Layout
        self.tf_layout.insertRow(0, "Interpolation", self.interpolation_combo)
        self.tf_layout.insertRow(0, "CDL Style", self.cdl_style_combo)
        self.tf_layout.insertRow(0, "CCC ID", self.ccc_id_edit)
        self.tf_layout.insertRow(0, "Source", self.src_edit)

    def transform(self) -> ocio.ColorSpaceTransform:
        transform = super().transform()
        transform.setSrc(self.src_edit.text())
        transform.setCCCId(self.ccc_id_edit.text())
        transform.setInterpolation(self.interpolation_combo.member())
        transform.setCDLStyle(self.cdl_style_combo.member())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        super().update_from_transform(transform)
        self.src_edit.setText(transform.getSrc())
        self.ccc_id_edit.setText(transform.getCCCId())
        self.interpolation_combo.set_member(transform.getInterpolation())
        self.cdl_style_combo.set_member(transform.getCDLStyle())

    def _on_src_changed(self):
        """
        Toggle file format specific widgets based on the file
        extension.
        """
        filename, ext = os.path.splitext(self.src_edit.text())
        self.ccc_id_edit.setEnabled(ext == ".ccc")
        self.cdl_style_combo.setEnabled(ext in (".cdl", ".cc", ".ccc"))


TransformEditFactory.register(ocio.FileTransform, FileTransformEdit)
