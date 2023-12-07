# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from PySide6 import QtCore, QtWidgets


class FormLayout(QtWidgets.QFormLayout):
    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)
        self.setLabelAlignment(QtCore.Qt.AlignRight)
