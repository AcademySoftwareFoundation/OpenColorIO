# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Callable, Optional, TYPE_CHECKING, Union

from PySide6 import QtCore, QtGui, QtWidgets

from ..utils import SignalsBlocked, next_name
from .item_view import BaseItemView

if TYPE_CHECKING:
    from ..items.config_item_model import BaseConfigItemModel

class StringListWidget(BaseItemView):
    """
    Simple string list widget with filter edit and add and remove
    buttons.
    """

    def __init__(
        self,
        item_basename: Optional[str] = None,
        item_flags: QtCore.Qt.ItemFlags = BaseItemView.DEFAULT_ITEM_FLAGS,
        item_icon: Optional[QtGui.QIcon] = None,
        allow_empty: bool = True,
        get_presets: Optional[Callable] = None,
        presets_only: bool = False,
        get_item: Optional[Callable] = None,
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param item_basename: Optional basename for prefixing new item
            names, formatted like: "<basename>_<number>". The number
            suffix is incremented so that all names are unique.
        :param item_flags: list item flags
        :param item_icon: Optional item icon
        :param allow_empty: If set to False, the remove button will do
            nothing when there is only one item.
        :param get_presets: Optional callback which returns either a
            list of string presets, or a dictionary of string presets
            and corresponding item icons, that can be selected from an
            add button popup menu.
        :param presets_only: When True, only preset items may be added.
            Clicking the add button will present the preset menu
            instead of adding an item to the view.
        :param get_item: Optional callback to request one new item from
            the user (e.g. via a dialog). The callback should return a
            string or ``None``, to indicate that no item should be
            added.
        """
        list_view = QtWidgets.QListWidget()
        list_view.itemChanged.connect(lambda i: self.items_changed.emit())
        list_view.currentRowChanged.connect(self.current_row_changed.emit)

        super().__init__(
            list_view,
            item_flags=item_flags,
            item_icon=item_icon,
            get_presets=get_presets,
            presets_only=presets_only,
            parent=parent,
        )

        self._item_basename = item_basename or ""
        self._allow_empty = allow_empty
        self._get_item = get_item

    # DataWidgetMapper user property interface
    @QtCore.Property(list, user=True)
    def __data(self) -> list[str]:
        return self.items()

    @__data.setter
    def __data(self, data: list[str]) -> None:
        with SignalsBlocked(self, self.view):
            self.set_items(data)

    def clear(self) -> None:
        self.view.clear()

    def items(self) -> list[str]:
        return [self.view.item(row).text() for row in range(self.view.count())]

    def set_current_item(self, text: str) -> tuple[bool, int]:
        items = self.view.findItems(text, QtCore.Qt.MatchExactly)
        if items:
            self.view.setCurrentItem(items[0])
            return True, self.view.currentRow()
        return False, self.view.currentRow()

    def add_item(self, text: Optional[str] = None) -> None:
        text = text or ""
        if self.view.findItems(text, QtCore.Qt.MatchExactly):
            # Item already exists
            return

        if self._item_icon is not None:
            item = QtWidgets.QListWidgetItem(self._item_icon, text)
        else:
            item = QtWidgets.QListWidgetItem(text)

        item.setFlags(self._item_flags)
        self.view.addItem(item)

        self.view.setCurrentItem(item)
        self.items_changed.emit()

    def remove_item(self, text_or_item: Union[str, QtWidgets.QListWidgetItem]) -> None:
        if isinstance(text_or_item, QtWidgets.QListWidgetItem):
            self.view.takeItem(self.view.row(text_or_item))
        else:
            for item in sorted(
                self.view.findItems(str(text_or_item), QtCore.Qt.MatchExactly),
                key=lambda i: self.view.row(i),
                reverse=True,
            ):
                self.view.takeItem(self.view.row(item))

        self.items_changed.emit()

    def set_items(self, items: list[str]) -> None:
        """
        Replace all items with the provided strings.

        :param items: list of item names
        """
        self.view.clear()

        if items:
            self.view.addItems(items)
            for row in range(self.view.count()):
                item = self.view.item(row)
                item.setFlags(self._item_flags)
                if self._item_icon is not None:
                    item.setIcon(self._item_icon)

        self.items_changed.emit()

    @QtCore.Slot(str)
    def _on_filter_text_changed(self, text: str) -> None:
        if len(text) < 2:
            for row in range(self.view.count()):
                self.view.setRowHidden(row, False)
            return

        for row in range(self.view.count()):
            self.view.setRowHidden(row, True)

        for item in self.view.findItems(text, QtCore.Qt.MatchContains):
            self.view.setRowHidden(self.view.row(item), False)

    def _on_add_button_released(self) -> None:
        if self._get_item is not None:
            # Use provided callback to get next name
            name = self._get_item()
            if name is not None:
                self.add_item(name)
            return

        # Generate next name from provided basename, or fallback to an empty item
        if self._item_basename:
            self.add_item(next_name(f"{self._item_basename}_", self.items()))
        else:
            self.add_item("")

    def _on_remove_button_released(self) -> None:
        for item in sorted(
            self.view.selectedItems(), key=lambda i: self.view.row(i), reverse=True
        ):
            if not self._allow_empty and self.view.count() == 1:
                QtWidgets.QMessageBox.warning(
                    self,
                    "Warning",
                    f"At least one {self._item_basename or 'item'} is required.",
                )
                continue
            self.remove_item(item)

    def _on_move_up_button_released(self) -> None:
        if self.view.selectedItems():
            src_row = self.view.currentRow()
            dst_row = max(0, src_row - 1)
            self._move_item(src_row, dst_row)

    def _on_move_down_button_released(self) -> None:
        if self.view.selectedItems():
            src_row = self.view.currentRow()
            dst_row = min(self.view.count() - 1, src_row + 1)
            self._move_item(src_row, dst_row)

    def _move_item(self, src_row: int, dst_row: int) -> None:
        src_item = self.view.takeItem(src_row)
        src_item_text = src_item.text()

        self.view.insertItem(dst_row, src_item)
        self.items_changed.emit()

        for dst_item in self.view.findItems(src_item_text, QtCore.Qt.MatchExactly):
            self.view.setItemSelected(dst_item, True)
            self.view.setCurrentRow(self.view.row(dst_item))
            break


class ListView(QtWidgets.QListView):
    current_row_changed = QtCore.Signal(int)

    def selectionChanged(
        self, selected: QtCore.QItemSelection, deselected: QtCore.QItemSelection
    ) -> None:
        # Emit last selected row
        indexes = selected.indexes()
        if indexes:
            self.current_row_changed.emit(indexes[-1].row())
        else:
            self.current_row_changed.emit(-1)


class ItemModelListWidget(BaseItemView):
    """list view with filter edit and add and remove buttons."""

    item_double_clicked = QtCore.Signal(QtCore.QModelIndex)

    def __init__(
        self,
        model: "BaseConfigItemModel",
        model_column: int,
        item_flags: QtCore.Qt.ItemFlags = BaseItemView.DEFAULT_ITEM_FLAGS,
        item_icon: Optional[QtGui.QIcon] = None,
        items_constant: bool = False,
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param model: list view model
        :param model_column: Model column to get values from
        :param item_flags: list item flags
        :param item_icon: Optional item icon
        :param items_constant: Optionally hide the add and remove
            buttons, for implementations where items are
            auto-populated. Note that preset support is dependent on
            this being False.
        """
        self._model = model
        self._model.dataChanged.connect(self._on_model_data_changed)
        self._model_column = model_column

        list_view = ListView()
        list_view.setModel(self._model)
        list_view.setModelColumn(self._model_column)
        list_view.current_row_changed.connect(self.current_row_changed.emit)
        list_view.doubleClicked.connect(self.item_double_clicked.emit)

        has_presets = self._model.has_presets()

        super().__init__(
            list_view,
            item_flags=item_flags,
            item_icon=item_icon,
            items_constant=items_constant,
            get_presets=None if not has_presets else self._model.get_presets,
            presets_only=self._model.requires_presets(),
            parent=parent,
        )

    def clear(self) -> None:
        row_count = self._model.rowCount()
        if row_count:
            self._model.removeRows(0, row_count)

    def items(self) -> list[str]:
        items = []
        for row in range(self._model.rowCount()):
            items.append(
                self._model.data(
                    self._model.index(row, self._model_column),
                    role=QtCore.Qt.DisplayRole,
                )
            )
        return items

    def set_current_item(self, text: str) -> tuple[bool, int]:
        indices = self._find_indices(text, hits=1)
        if indices:
            index = indices[0]
            row = index.row()
            self.view.setCurrentIndex(index)
            self.current_row_changed.emit(row)
            return True, row
        else:
            return False, self.current_row()

    def current_index(self) -> Optional[QtCore.QModelIndex]:
        """
        :return: Current model index
        """
        return self.view.currentIndex()

    def current_row(self) -> int:
        """
        :return: Current list row
        """
        return self.current_index().row()

    def set_current_row(self, row: int) -> None:
        """
        :param row: Make the specified row current
        """
        if row < self._model.rowCount():
            self.view.setCurrentIndex(self._model.index(row, self._model_column))

    def add_item(self, text: Optional[str] = None) -> None:
        item_row = -1
        if self._has_presets and text is not None:
            # Try to create preset item
            item_row = self._model.add_preset(text)

        if item_row == -1:
            item_row = self._model.create_item(text)

        if item_row != -1:
            self.set_current_row(item_row)

    def remove_item(self, text: str) -> None:
        indices = self._find_indices(text)
        if indices:
            for index in indices:
                self._model.removeRows(index.row(), 1)

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
        selection_model = self.view.selectionModel()
        for index in sorted(
            selection_model.selectedIndexes(), key=lambda i: i.row(), reverse=True
        ):
            self._model.removeRows(index.row(), 1)

    def _on_move_up_button_released(self) -> None:
        current_index = self.view.currentIndex()
        name = self._model.data(current_index, QtCore.Qt.DisplayRole)
        self._model.move_item_up(name)

    def _on_move_down_button_released(self) -> None:
        current_index = self.view.currentIndex()
        name = self._model.data(current_index, QtCore.Qt.DisplayRole)
        self._model.move_item_down(name)

    @QtCore.Slot(QtCore.QModelIndex, QtCore.QModelIndex, list)
    def _on_model_data_changed(
        self,
        top_left: QtCore.QModelIndex,
        bottom_right: QtCore.QModelIndex,
        roles: list[QtCore.Qt.ItemDataRole] = (),
    ):
        """
        Called when the data for one or more indices in the model
        changes.
        """
        if top_left.column() == self._model_column:
            self.items_changed.emit()

    def _find_indices(
        self,
        text: str,
        hits: int = -1,
        flags: QtCore.Qt.MatchFlags = QtCore.Qt.MatchExactly,
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
            return self._model.match(
                self._model.index(0, self._model_column),
                QtCore.Qt.DisplayRole,
                text,
                hits=hits,
                flags=flags,
            )
