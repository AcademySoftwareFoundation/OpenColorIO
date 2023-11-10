# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from PySide6 import QtCore, QtWidgets

from ..utils import SignalsBlocked


class CheckBox(QtWidgets.QCheckBox):
    # DataWidgetMapper user property interface
    @QtCore.Property(bool, user=True)
    def __data(self) -> bool:
        return self.value()

    @__data.setter
    def __data(self, data: bool) -> None:
        with SignalsBlocked(self):
            self.set_value(data)

    # Common public interface
    def value(self) -> bool:
        return self.isChecked()

    def set_value(self, value: bool) -> None:
        self.setChecked(value)

    def reset(self) -> None:
        self.setChecked(False)
