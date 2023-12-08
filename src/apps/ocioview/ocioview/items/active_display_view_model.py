# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from dataclasses import dataclass
from typing import Any, Callable, Optional, Type

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui

from ..config_cache import ConfigCache
from ..undo import ConfigSnapshotUndoCommand
from .config_item_model import ColumnDesc, BaseConfigItemModel


@dataclass
class Display:
    """Individual display storage."""

    name: str
    active: False


@dataclass
class View:
    """Individual view storage."""

    name: str
    active: False


class BaseActiveDisplayViewModel(BaseConfigItemModel):
    """
    Base item model for active displays and views in the current
    config.
    """

    ACTIVE = ColumnDesc(1, "Active", bool)

    # OCIO config object type this model manages.
    __item_type__: type = None

    # Callable to get all items from the config cache
    __get_all_items__: Callable = None

    # Callable to get active items from the config cache
    __get_active_items__: Callable = None

    # Config attribute name for method to set the active item string
    __set_active_items_attr__: str = None

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        ConfigCache.register_reset_callback(self._reset_cache)

    def move_item_up(self, item_name: str) -> bool:
        active_names = self.__get_active_items__()
        if item_name not in active_names:
            return False

        src_row = active_names.index(item_name)
        dst_row = max(0, src_row - 1)

        if dst_row == src_row:
            return False

        return self.moveRows(self.NULL_INDEX, src_row, 1, self.NULL_INDEX, dst_row)

    def move_item_down(self, item_name: str) -> bool:
        active_names = self.__get_active_items__()
        if item_name not in active_names:
            return False

        src_row = active_names.index(item_name)
        dst_row = min(len(active_names) - 1, src_row + 1)

        if dst_row == src_row:
            return False

        return self.moveRows(self.NULL_INDEX, src_row, 1, self.NULL_INDEX, dst_row)

    def flags(self, index: QtCore.QModelIndex) -> int:
        return super().flags(index) | QtCore.Qt.ItemIsUserCheckable

    def get_item_names(self) -> list[str]:
        return [item.name for item in self._get_items()]

    def _get_undo_command_type(
        self, column_desc: ColumnDesc
    ) -> Type[QtGui.QUndoCommand]:
        if column_desc == self.ACTIVE:
            # Changing check state of the ACTIVE column has side effects related to
            # display/view order, so a config snapshot is needed to revert the change.
            return ConfigSnapshotUndoCommand
        else:
            return super()._get_undo_command_type(column_desc)

    def _get_icon(
        self, item: __item_type__, column_desc: ColumnDesc
    ) -> Optional[QtGui.QIcon]:
        if column_desc == self.NAME:
            return self.item_type_icon()
        else:
            return None

    def _reset_cache(self) -> None:
        self._items = []

    def _get_items(self, preserve: bool = False) -> list[__item_type__]:
        if ConfigCache.validate() and self._items:
            return self._items

        all_names = self.__get_all_items__()
        active_names = self.__get_active_items__()

        self._items = []

        for name in active_names:
            self._items.append(self.__item_type__(name, True))
        for name in all_names:
            if name not in active_names:
                self._items.append(self.__item_type__(name, False))

        return self._items

    def _clear_items(self) -> None:
        getattr(ocio.GetCurrentConfig(), self.__set_active_items_attr__)("")

    def _add_item(self, item: __item_type__) -> None:
        active_names = self.__get_active_items__()
        if item.active and item.name not in active_names:
            active_names.append(item.name)
        getattr(ocio.GetCurrentConfig(), self.__set_active_items_attr__)(
            ",".join(active_names)
        )

    def _remove_item(self, item: __item_type__) -> None:
        active_names = self.__get_active_items__()
        if not item.active and item.name in active_names:
            active_names.remove(item.name)
        getattr(ocio.GetCurrentConfig(), self.__set_active_items_attr__)(
            ",".join(active_names)
        )

    def _new_item(self, name: __item_type__) -> None:
        # Existing config items only
        pass

    def _get_checked_column(self) -> Optional[ColumnDesc]:
        return self.ACTIVE

    def _get_value(self, item: __item_type__, column_desc: ColumnDesc) -> Any:
        # Get parameters
        if column_desc == self.NAME:
            return item.name
        elif column_desc == self.ACTIVE:
            return item.active

        # Invalid column
        return None

    def _set_value(
        self,
        item: __item_type__,
        column_desc: ColumnDesc,
        value: Any,
        index: QtCore.QModelIndex,
    ) -> None:
        # Update parameters
        if column_desc == self.ACTIVE:
            item.active = value
            if value is True:
                self._add_item(item)
                self.item_added.emit(item.name)
            else:
                self._remove_item(item)
                self.item_removed.emit()


class ActiveDisplayModel(BaseActiveDisplayViewModel):
    """
    Item model for active displays in the current config.
    """

    NAME = ColumnDesc(0, "Display", str)

    COLUMNS = [NAME, BaseActiveDisplayViewModel.ACTIVE]

    __item_type__ = Display
    __icon_glyph__ = "mdi6.monitor"
    __get_active_items__ = ConfigCache.get_active_displays
    __get_all_items__ = ConfigCache.get_displays
    __set_active_items_attr__ = "setActiveDisplays"


class ActiveViewModel(BaseActiveDisplayViewModel):
    """
    Item model for active views in the current config.
    """

    NAME = ColumnDesc(0, "View", str)

    COLUMNS = [NAME, BaseActiveDisplayViewModel.ACTIVE]

    __item_type__ = View
    __icon_glyph__ = "mdi6.eye-outline"
    __get_active_items__ = ConfigCache.get_active_views
    __get_all_items__ = ConfigCache.get_views
    __set_active_items_attr__ = "setActiveViews"
