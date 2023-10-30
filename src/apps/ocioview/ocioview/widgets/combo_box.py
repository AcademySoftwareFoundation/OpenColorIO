# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import enum
from typing import Callable, Optional

from PySide6 import QtCore, QtGui, QtWidgets

from ..utils import SignalsBlocked


class ComboBox(QtWidgets.QComboBox):
    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)
        self.setSizeAdjustPolicy(QtWidgets.QComboBox.AdjustToMinimumContentsLengthWithIcon)

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
        return self.currentText()

    def set_value(self, value: str) -> None:
        self.setCurrentText(value)

    def reset(self) -> None:
        if self.isEditable():
            self.setEditText("")
        elif self.count():
            self.setCurrentIndex(0)


class EnumComboBox(ComboBox):
    """Combo box with an enum model."""

    def __init__(
        self,
        enum_type: enum.Enum,
        icons: Optional[dict[enum.Enum, QtGui.QIcon]] = None,
        parent: Optional[QtCore.QObject] = None,
    ):
        super().__init__(parent=parent)

        for name, member in enum_type.__members__.items():
            if icons is not None and member in icons:
                self.addItem(icons[member], name, userData=member)
            else:
                self.addItem(name, userData=member)

    # DataWidgetMapper user property interface
    @QtCore.Property(int, user=True)
    def __data(self) -> int:
        return self.currentIndex()

    @__data.setter
    def __data(self, data: int) -> None:
        with SignalsBlocked(self):
            self.setCurrentIndex(data)

    # Direct enum member access
    def member(self) -> enum.Enum:
        return self.currentData()

    def set_member(self, value: enum.Enum) -> None:
        self.setCurrentText(value.name)


class CallbackComboBox(ComboBox):
    """Combo box modeled around provided item callback(s)."""

    def __init__(
        self,
        get_items: Callable,
        get_default_item: Optional[Callable] = None,
        item_icon: Optional[QtGui.QIcon] = None,
        editable: bool = False,
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param get_items: Required callback which receives no
            parameters and returns a list of item strings, or a
            dictionary with item string keys and QIcon values, to add
            to combo box.
        :param get_default_item: Optional callback which receives no
            parameters and returns the default item string. If unset,
            the first item is the default.
        :param item_icon: Optionally provide one static icon for all
            items. Icons provided by 'get_items' take precedence.
        :param editable: Whether combo box is editable
        """
        super().__init__(parent=parent)

        self._get_items = get_items
        self._get_default_item = get_default_item
        self._item_icon = item_icon

        self.setEditable(editable)
        self.setInsertPolicy(QtWidgets.QComboBox.NoInsert)

        completer = self.completer()
        if completer is not None:
            completer.setCompletionMode(QtWidgets.QCompleter.PopupCompletion)

        # Initialize
        self.update_items()

    # DataWidgetMapper user property interface
    @QtCore.Property(str, user=True)
    def __data(self) -> str:
        return self.value()

    @__data.setter
    def __data(self, data: str) -> None:
        with SignalsBlocked(self):
            if self.findText(data) == -1:
                self.update_items()
            self.set_value(data)

    def update_items(self) -> str:
        """
        Call the provided callback(s) to update combo box items.

        :return: Current item string
        """
        # Get current state
        current_item = None
        if not self.count():
            if self._get_default_item is not None:
                current_item = self._get_default_item()
        else:
            current_item = self.currentText()

        # Reload all items
        with SignalsBlocked(self):
            self.clear()
            items = self._get_items()
            if isinstance(items, dict):
                for item, icon in items.items():
                    if icon is None:
                        icon = self._item_icon
                    self.addItem(icon, item)
            else:
                if self._item_icon is not None:
                    for item in self._get_items():
                        self.addItem(self._item_icon, item)
                else:
                    self.addItems(self._get_items())

        # Restore original state
        index = self.findText(current_item)
        if index != -1:
            self.setCurrentIndex(index)
        elif self._get_default_item is not None:
            self.setCurrentText(self._get_default_item())

        return self.currentText()

    def showPopup(self) -> None:
        """
        Reload items whenever the popup is shown for just-in-time
        model updates.
        """
        text = self.update_items()

        super().showPopup()

        # This selects the current item in the popup and must be called after the
        # popup is shown.
        items = self.model().findItems(text)
        if items:
            self.view().setCurrentIndex(items[0].index())

    def reset(self) -> None:
        super().reset()
        self.update_items()
