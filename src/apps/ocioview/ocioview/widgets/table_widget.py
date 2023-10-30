# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Callable, Optional

from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import ICON_SIZE_ITEM
from ..utils import SignalsBlocked, next_name
from .item_view import BaseItemView


class StringMapTableWidget(BaseItemView):
    """
    Two-column table widget with filter edit and add and remove
    buttons.
    """

    def __init__(
        self,
        header_labels: tuple[str, str],
        data: Optional[dict] = None,
        item_icon: Optional[QtGui.QIcon] = None,
        default_key_prefix: str = "",
        default_value: str = "",
        parent: Optional[QtWidgets.QWidget] = None,
    ):
        """
        :param header_labels: Labels for each column
        :param data: Optional dictionary to pre-populate table from
        """
        super().__init__(
            item_view=QtWidgets.QTableWidget(),
            item_icon=item_icon,
            items_movable=False,
            parent=parent,
        )

        self._header_labels = header_labels
        self._default_key_prefix = default_key_prefix
        self._default_value = default_value

        self.view.setColumnCount(2)
        self.view.setSelectionBehavior(QtWidgets.QTableWidget.SelectRows)
        self.view.horizontalHeader().setStretchLastSection(True)
        self.view.setHorizontalHeaderLabels(self._header_labels)

        vertical_header = self.view.verticalHeader()
        vertical_header.setDefaultSectionSize(ICON_SIZE_ITEM.height())
        vertical_header.hide()

        self.view.itemChanged.connect(self._on_view_item_changed)

        if data is not None:
            self.set_items(data)

    # DataWidgetMapper user property interface
    # NOTE: A list of tuples is used here instead of a dictionary, as passing a
    #       dictionary through DataWidgetMapper results in `None` being received
    #       by the model.
    @QtCore.Property(list, user=True)
    def __data(self) -> list[tuple[str, str]]:
        return [(k, v) for k, v in self.items().items()]

    @__data.setter
    def __data(self, data: list[tuple[str, str]]) -> None:
        with SignalsBlocked(self, self.view):
            self.set_items({k: v for k, v in data})

    def clear(self) -> None:
        self.view.setRowCount(0)

    def items(self) -> dict[str, str]:
        data = {}
        for row in range(self.view.rowCount()):
            key_item = self.view.item(row, 0)
            value_item = self.view.item(row, 1)

            # Key is required, but value defaults to ""
            if key_item is not None:
                key = key_item.text()
                if key:
                    if value_item is not None:
                        value = value_item.text()
                    else:
                        value = ""

                    data[key] = value
        return data

    def set_items(self, data: dict[str, str]) -> None:
        """
        Reset table with the provided data.

        :param data: Map key, value pairs
        """
        # Clear table without resetting horizontal header items
        self.view.setRowCount(0)
        self.view.setRowCount(len(data))

        for row, (key, value) in enumerate(data.items()):
            key_item = QtWidgets.QTableWidgetItem(str(key))
            if self._item_icon is not None:
                key_item.setIcon(self._item_icon)
            self.view.setItem(row, 0, key_item)
            value_item = QtWidgets.QTableWidgetItem(str(value))
            self.view.setItem(row, 1, value_item)

    def add_item(self, key: Optional[str] = None, value: Optional[str] = None) -> None:
        row = self.view.rowCount()
        self.view.setRowCount(row + 1)

        key_item = QtWidgets.QTableWidgetItem(
            str(
                key
                or (
                    ""
                    if not self._default_key_prefix
                    else next_name(self._default_key_prefix, list(self.items().keys()))
                )
            )
        )
        if self._item_icon is not None:
            key_item.setIcon(self._item_icon)

        value_item = QtWidgets.QTableWidgetItem(str(value or self._default_value))

        # Don't emit items changed signal until both key and value have been set, since
        # some underlying models will invalidate rows with only key or value set.
        with SignalsBlocked(self):
            self.view.setItem(row, 0, key_item)
            self.view.setItem(row, 1, value_item)

        self.items_changed.emit()

    def remove_item(self, text: str) -> None:
        remove_rows = set()

        for item in self.view.findItems(text, QtCore.Qt.MatchExactly):
            remove_rows.add(item.row())

        for row in sorted(remove_rows, reverse=True):
            self.view.removeRow(row)

        # Removing items doesn't emit the builtin itemChanged signal
        self.items_changed.emit()

    def set_current_item(self, text: str) -> tuple[bool, int]:
        items = self.view.findItems(text, QtCore.Qt.MatchExactly)
        if items:
            item = items[0]
            row = item.row()
            self.view.selectRow(row)
            return True, row
        else:
            return False, self.view.currentIndex().row()

    @QtCore.Slot(str)
    def _on_filter_text_changed(self, text: str) -> None:
        if len(text) < 2:
            for row in range(self.view.rowCount()):
                self.view.setRowHidden(row, False)
            return

        for row in range(self.view.rowCount()):
            self.view.setRowHidden(row, True)

        for item in self.view.findItems(text, QtCore.Qt.MatchContains):
            self.view.setRowHidden(item.row(), False)

    def _on_add_button_released(self) -> None:
        self.add_item()

    def _on_remove_button_released(self) -> None:
        remove_rows = set()

        for item in self.view.selectedItems():
            remove_rows.add(item.row())

        for row in sorted(remove_rows, reverse=True):
            self.view.removeRow(row)

        # Removing items doesn't emit the builtin itemChanged signal
        self.items_changed.emit()

    def _on_view_item_changed(self, *args, **kwargs) -> None:
        """Notify watchers of item changes"""
        self.items_changed.emit()


class ItemModelTableWidget(BaseItemView):
    """Table view with filter edit and add and remove buttons."""

    def __init__(
        self,
        model: QtCore.QAbstractTableModel,
        get_presets: Optional[Callable] = None,
        presets_only: bool = False,
        parent: Optional[QtWidgets.QWidget] = None,
    ):
        """
        :param model: Table model
        """
        super().__init__(
            item_view=QtWidgets.QTableView(),
            items_movable=False,
            get_presets=get_presets,
            presets_only=presets_only,
            parent=parent,
        )

        self._model = model
        self._model.dataChanged.connect(self._on_view_item_changed)
        self._model.item_added.connect(self._on_item_added)

        self.view.setModel(self._model)
        self.view.setHorizontalScrollMode(QtWidgets.QTableWidget.ScrollPerPixel)
        self.view.setSelectionBehavior(QtWidgets.QTableWidget.SelectRows)
        self.view.selectionModel().currentRowChanged.connect(
            lambda current, previous: self.current_row_changed.emit(current.row())
        )

        horizontal_header = self.view.horizontalHeader()
        horizontal_header.setStretchLastSection(True)
        horizontal_header.setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)

        vertical_header = self.view.verticalHeader()
        vertical_header.setDefaultSectionSize(ICON_SIZE_ITEM.height())
        vertical_header.hide()

    def clear(self) -> None:
        row_count = self._model.rowCount()
        if row_count:
            self._model.removeRows(0, row_count)

    def items(self) -> list[list[str]]:
        items = []
        column_count = self._model.columnCount()

        for row in range(self._model.rowCount()):
            item_data = []
            for column in range(column_count):
                item_data.append(
                    self._model.data(
                        self._model.index(row, column),
                        role=QtCore.Qt.DisplayRole,
                    )
                )
            items.append(item_data)

        return items

    def add_item(self, text: Optional[str] = None) -> None:
        item_row = -1
        if self._has_presets and text is not None:
            # Try to create preset item
            item_row = self._model.add_preset(text)

        if item_row == -1:
            # Let model create a default item
            if self._model.insertRows(0, 1):
                item_row = 0

        self.set_current_row(item_row)

    def remove_item(self, text: str) -> None:
        remove_rows = set()

        for index in self._find_indices(text, flags=QtCore.Qt.MatchExactly):
            remove_rows.add(index.row())

        for row in sorted(remove_rows, reverse=True):
            self._model.removeRows(row, 1)

        # Removing items doesn't emit the builtin itemChanged signal
        self.items_changed.emit()

    def set_current_item(self, text: str) -> tuple[bool, int]:
        indices = self._find_indices(text, hits=1, flags=QtCore.Qt.MatchExactly)
        if indices:
            index = indices[0]
            row = index.row()
            self.view.selectRow(row)
            return True, row
        else:
            return False, self.view.currentIndex().row()

    def set_current_row(self, row: int) -> None:
        """
        :param row: Make the specified row current
        """
        self.view.selectRow(row)

    @QtCore.Slot(QtCore.QModelIndex)
    def _on_item_added(self, name: str) -> None:
        """Set the most recently added item as current/selected."""
        for index in self._find_indices(name):
            self.view.setCurrentIndex(index)

    @QtCore.Slot(str)
    def _on_filter_text_changed(self, text: str) -> None:
        if len(text) < 2:
            for row in range(self._model.rowCount()):
                self.view.setRowHidden(row, False)
            return

        for row in range(self._model.rowCount()):
            self.view.setRowHidden(row, True)

        for index in self._find_indices(text, flags=QtCore.Qt.MatchContains):
            self.view.setRowHidden(index.row(), False)

    def _on_add_button_released(self) -> None:
        self.add_item()

    def _on_remove_button_released(self) -> None:
        remove_rows = set()

        for index in self.view.selectedIndexes():
            remove_rows.add(index.row())

        for row in sorted(remove_rows, reverse=True):
            self._model.removeRows(row, 1)

        # Removing items doesn't emit the builtin itemChanged signal
        self.items_changed.emit()

    def _on_move_up_button_released(self) -> None:
        current_index = self.view.currentIndex()
        parent = current_index.parent()
        row = current_index.row()

        self.view.model().moveRow(parent, row, parent, row - 1)

    def _on_move_down_button_released(self) -> None:
        current_index = self.view.currentIndex()
        parent = current_index.parent()
        row = current_index.row()

        self.view.model().moveRow(parent, row, parent, row + 1)

    def _on_view_item_changed(self, *args, **kwargs) -> None:
        """Notify watchers of item changes"""
        self.items_changed.emit()

    def _find_indices(
        self,
        text: str,
        hits: int = -1,
        flags: QtCore.Qt.MatchFlags = QtCore.Qt.MatchFixedString | QtCore.Qt.MatchWrap,
    ) -> list[QtCore.QModelIndex]:
        """
        Search the model for items matching the provided text and
        flags.

        :param text: Text to search for in model column
        :param hits: Optional maximum number of items to find. Defaults
            to find all items.
        :param flags: Match flags
        :return: list of matching model indices
        """
        if not self._model.rowCount():
            return []
        else:
            flags |= QtCore.Qt.MatchWrap

            rows = set()
            for column in range(self._model.columnCount()):
                indices = self._model.match(
                    self._model.index(0, column),
                    QtCore.Qt.DisplayRole,
                    text,
                    hits=hits,
                    flags=flags,
                )
                rows.update(index.row() for index in indices)

            return [self._model.index(row, 0) for row in sorted(rows)]
