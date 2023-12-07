# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtWidgets

from ..config_cache import ConfigCache
from ..widgets import CallbackComboBox
from .role_model import RoleModel


class ColorSpaceDelegate(QtWidgets.QStyledItemDelegate):
    """
    Delegate for choosing a color space directly within a list view.
    """

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Keep track of previous value to detect duplicate entries
        self._prev_value = None

    def createEditor(
        self,
        parent: QtWidgets.QWidget,
        option: QtWidgets.QStyleOptionViewItem,
        index: QtCore.QModelIndex,
    ) -> QtWidgets.QWidget:
        """
        Create searchable combo box with available options/presets.
        """
        editor = CallbackComboBox(
            get_items=ConfigCache.get_color_space_names, editable=True, parent=parent
        )
        editor.completer().setCompletionMode(QtWidgets.QCompleter.PopupCompletion)
        return editor

    def setEditorData(
        self, editor: QtWidgets.QWidget, index: QtCore.QModelIndex
    ) -> None:
        """Pull data from model."""
        self._prev_value = index.data(QtCore.Qt.EditRole)
        editor.setCurrentText(self._prev_value)
        editor.lineEdit().selectAll()

    def updateEditorGeometry(
        self,
        editor: QtWidgets.QWidget,
        option: QtWidgets.QStyleOptionViewItem,
        index: QtCore.QModelIndex,
    ) -> None:
        """Position delegate widget directly over table cell."""
        editor.setGeometry(option.rect)

    def setModelData(
        self,
        editor: QtWidgets.QWidget,
        model: QtCore.QAbstractItemModel,
        index: QtCore.QModelIndex,
    ) -> None:
        """Validate and push data back to model."""
        value = editor.currentText()
        model = index.model()

        # Verify that this color space is not already present in the model
        if value != self._prev_value:
            other_indices = model.match(
                model.index(0, 0),
                QtCore.Qt.DisplayRole,
                value,
                flags=QtCore.Qt.MatchExactly | QtCore.Qt.MatchWrap,
            )
            if len(other_indices) > 1:
                return

        # Verify manually entered value is a color space
        config = ocio.GetCurrentConfig()
        color_space = config.getColorSpace(value)
        if color_space is None:
            return

        model.setData(index, value, QtCore.Qt.EditRole)


class RoleDelegate(QtWidgets.QStyledItemDelegate):
    """
    Delegate for editing role names and color spaces directly within
    a role table view.
    """

    def __init__(self, model: RoleModel, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Keep model reference to check existing roles in use
        self._model = model

        # Keep track of previous value to detect duplicate roles
        self._prev_value = None

    def createEditor(
        self,
        parent: QtWidgets.QWidget,
        option: QtWidgets.QStyleOptionViewItem,
        index: QtCore.QModelIndex,
    ) -> QtWidgets.QWidget:
        """
        Create searchable combo box with available options/presets.
        """
        column = index.column()

        if column == RoleModel.NAME.column:
            get_items = self._get_available_roles
        elif column == RoleModel.COLOR_SPACE.column:
            get_items = ConfigCache.get_color_space_names
        else:
            raise NotImplementedError

        widget = CallbackComboBox(get_items=get_items, editable=True, parent=parent)
        widget.completer().setCompletionMode(QtWidgets.QCompleter.PopupCompletion)
        return widget

    def setEditorData(
        self, editor: QtWidgets.QWidget, index: QtCore.QModelIndex
    ) -> None:
        """Pull data from model."""
        self._prev_value = index.data(QtCore.Qt.EditRole)
        editor.setCurrentText(self._prev_value)
        editor.lineEdit().selectAll()

    def updateEditorGeometry(
        self,
        editor: QtWidgets.QWidget,
        option: QtWidgets.QStyleOptionViewItem,
        index: QtCore.QModelIndex,
    ) -> None:
        """Position delegate widget directly over table cell."""
        editor.setGeometry(option.rect)

    def setModelData(
        self,
        editor: QtWidgets.QWidget,
        model: QtCore.QAbstractItemModel,
        index: QtCore.QModelIndex,
    ) -> None:
        """Validate and push data back to model."""
        value = editor.currentText()
        model = index.model()
        column = index.column()

        if column == RoleModel.NAME.column:
            # Verify that this role is not already set by a different item
            if value != self._prev_value:
                other_indices = model.match(
                    model.index(0, 0),
                    QtCore.Qt.DisplayRole,
                    value,
                    flags=QtCore.Qt.MatchExactly | QtCore.Qt.MatchWrap,
                )
                if len(other_indices) > 1:
                    return

        elif column == RoleModel.COLOR_SPACE.column:
            # Verify manually entered value is a color space
            config = ocio.GetCurrentConfig()
            color_space = config.getColorSpace(value)
            if color_space is None:
                return

        else:
            raise NotImplementedError

        model.setData(index, value, QtCore.Qt.EditRole)

    def _get_available_roles(self) -> list[str]:
        """
        :return: All builtin color space roles that aren't already
            defined by the model, for use in the preset menu.
        """
        defined_roles = []
        for i in range(self._model.rowCount()):
            defined_roles.append(
                self._model.data(self._model.index(i, 0), QtCore.Qt.EditRole)
            )

        # Include current value to allow easy bailing from preset menu
        return [self._prev_value] + [
            role
            for role in ConfigCache.get_builtin_color_space_roles()
            if role not in defined_roles
        ]
