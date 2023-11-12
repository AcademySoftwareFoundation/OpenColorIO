# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from PySide6 import QtCore, QtWidgets

from ..utils import SignalsBlocked


class TextEdit(QtWidgets.QPlainTextEdit):
    def __init__(
        self, text: Optional[str] = None, parent: Optional[QtCore.QObject] = None
    ):
        super().__init__(parent=parent)
        self.setLineWrapMode(QtWidgets.QPlainTextEdit.NoWrap)

        if text is not None:
            self.set_value(text)

    # DataWidgetMapper user property interface
    @QtCore.Property(str, user=True)
    def __data(self) -> str:
        return self.value()

    @__data.setter
    def __data(self, data: str) -> None:
        with SignalsBlocked(self):
            self.set_value(data)

    # Common public interface
    def value(self) -> str:
        return self.toPlainText()

    def set_value(self, value: str) -> None:
        self.setPlainText(value)

    def reset(self) -> None:
        self.clear()


class HtmlView(QtWidgets.QTextEdit):
    def __init__(
        self, text: Optional[str] = None, parent: Optional[QtCore.QObject] = None
    ):
        super().__init__(parent=parent)
        self.setLineWrapMode(QtWidgets.QTextEdit.NoWrap)
        self.setReadOnly(True)
        self.setUndoRedoEnabled(False)

        if text is not None:
            self.set_value(text)

    # DataWidgetMapper user property interface
    @QtCore.Property(str, user=True)
    def __data(self) -> str:
        return self.value()

    @__data.setter
    def __data(self, data: str) -> None:
        with SignalsBlocked(self):
            self.set_value(data)

    # Common public interface
    def value(self) -> str:
        return self.toHtml()

    def set_value(self, value: str) -> None:
        self.setHtml(value)

    def reset(self) -> None:
        self.clear()
