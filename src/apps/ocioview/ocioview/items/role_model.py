# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from dataclasses import dataclass
from typing import Any

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..config_cache import ConfigCache
from .config_item_model import ColumnDesc, BaseConfigItemModel


@dataclass
class Role:
    """Individual role storage."""

    name: str
    color_space: str


class RoleModel(BaseConfigItemModel):
    """
    Item model for color space roles in the current config.
    """

    NAME = ColumnDesc(0, "Name", str)
    COLOR_SPACE = ColumnDesc(1, "Color Space", str)

    COLUMNS = [NAME, COLOR_SPACE]

    __item_type__ = Role
    __icon_glyph__ = "ph.tag"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        ConfigCache.register_reset_callback(self._reset_cache)

    def flags(self, index: QtCore.QModelIndex) -> int:
        return (
            QtCore.Qt.ItemIsSelectable
            | QtCore.Qt.ItemIsEnabled
            | QtCore.Qt.ItemIsEditable
        )

    def get_item_names(self) -> list[str]:
        return [item.name for item in self._get_items()]

    def _reset_cache(self) -> None:
        self._items = []

    def _get_items(self, preserve: bool = False) -> list[Role]:
        if ConfigCache.validate() and self._items:
            return self._items

        self._items = [Role(*role) for role in ocio.GetCurrentConfig().getRoles()]
        return self._items

    def _clear_items(self) -> None:
        config = ocio.GetCurrentConfig()
        for name in config.getRoleNames():
            # Unset role
            config.setRole(name, None)

    def _add_item(self, item: Role) -> None:
        ocio.GetCurrentConfig().setRole(item.name, item.color_space)

    def _remove_item(self, item: Role) -> None:
        # Unset role
        ocio.GetCurrentConfig().setRole(item.name, None)

    def _new_item(self, name: str) -> None:
        color_space_names = ConfigCache.get_color_space_names(
            ocio.SEARCH_REFERENCE_SPACE_SCENE
        )
        if color_space_names:
            ocio.GetCurrentConfig().setRole(name, color_space_names[0])

    def _get_value(self, item: Role, column_desc: ColumnDesc) -> Any:
        # Get parameters
        if column_desc == self.NAME:
            return item.name
        elif column_desc == self.COLOR_SPACE:
            return item.color_space

        # Invalid column
        return None

    def _set_value(
        self, item: Role, column_desc: ColumnDesc, value: Any, index: QtCore.QModelIndex
    ) -> None:
        config = ocio.GetCurrentConfig()
        prev_item_name = item.name

        # Update parameters
        if column_desc == self.NAME:
            # Unset previous role name
            config.setRole(prev_item_name, None)
            item.name = value
        elif column_desc == self.COLOR_SPACE:
            item.color_space = value

        config.setRole(item.name, item.color_space)

        if item.name != prev_item_name:
            # Tell views to follow selection to new item
            self.item_added.emit(item.name)

            self.item_renamed.emit(item.name, prev_item_name)
