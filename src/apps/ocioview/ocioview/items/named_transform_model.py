# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import copy
from typing import Any, Optional, Union

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui

from ..config_cache import ConfigCache
from ..utils import get_glyph_icon
from .config_item_model import ColumnDesc, BaseConfigItemModel


class NamedTransformModel(BaseConfigItemModel):
    """
    Item model for editing named transforms in the current config.
    """

    NAME = ColumnDesc(0, "Name", str)
    ALIASES = ColumnDesc(1, "Aliases", list)
    FAMILY = ColumnDesc(2, "Family", str)
    ENCODING = ColumnDesc(3, "Encoding", str)
    DESCRIPTION = ColumnDesc(4, "Description", str)
    CATEGORIES = ColumnDesc(5, "Categories", list)
    FORWARD_TRANSFORM = ColumnDesc(6, "Forward Transform", ocio.Transform)
    INVERSE_TRANSFORM = ColumnDesc(7, "Inverse Transform", ocio.Transform)

    # fmt: off
    COLUMNS = sorted([
        NAME, ALIASES, FAMILY, ENCODING, DESCRIPTION,  CATEGORIES,
        FORWARD_TRANSFORM, INVERSE_TRANSFORM,
    ], key=lambda s: s.column)
    # fmt: on

    __item_type__ = ocio.NamedTransform
    __icon_glyph__ = "ph.arrow-square-right"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        self._item_icon = get_glyph_icon("ph.arrow-square-right")

    def get_item_names(self) -> list[str]:
        return [item.getName() for item in self._get_items()]

    def get_item_transforms(
        self, item_name: str
    ) -> tuple[Optional[ocio.Transform], Optional[ocio.Transform]]:
        # Get view name from subscription item name
        item_name = self.extract_subscription_item_name(item_name)

        config = ocio.GetCurrentConfig()
        named_transform = config.getNamedTransform(item_name)
        if named_transform is not None:

            fwd_tf = named_transform.getTransform(ocio.TRANSFORM_DIR_FORWARD)
            if not fwd_tf:
                inv_tf_ = named_transform.getTransform(ocio.TRANSFORM_DIR_INVERSE)
                if inv_tf_:
                    fwd_tf = ocio.GroupTransform([inv_tf_], ocio.TRANSFORM_DIR_INVERSE)

            inv_tf = named_transform.getTransform(ocio.TRANSFORM_DIR_INVERSE)
            if not inv_tf:
                fwd_tf_ = named_transform.getTransform(ocio.TRANSFORM_DIR_FORWARD)
                if fwd_tf_:
                    inv_tf = ocio.GroupTransform([fwd_tf_], ocio.TRANSFORM_DIR_INVERSE)

            return fwd_tf, inv_tf
        else:
            return None, None

    def _get_icon(
        self, item: ocio.ColorSpace, column_desc: ColumnDesc
    ) -> Optional[QtGui.QIcon]:
        return self._get_subscription_icon(item, column_desc) or super()._get_icon(
            item, column_desc
        )

    def _get_bg_color(
        self, item: __item_type__, column_desc: ColumnDesc
    ) -> Optional[QtGui.QColor]:
        if column_desc == self.NAME:
            return self._get_subscription_color(item, column_desc)
        else:
            return None

    def _get_items(self, preserve: bool = False) -> list[ocio.ColorSpace]:
        if preserve:
            self._items = [
                copy.deepcopy(item) for item in ConfigCache.get_named_transforms()
            ]
            return self._items
        else:
            return ConfigCache.get_named_transforms()

    def _clear_items(self) -> None:
        ocio.GetCurrentConfig().clearNamedTransforms()

    def _add_item(self, item: ocio.NamedTransform) -> None:
        ocio.GetCurrentConfig().addNamedTransform(item)

    def _remove_item(self, item: ocio.NamedTransform) -> None:
        config = ocio.GetCurrentConfig()
        items = [
            copy.deepcopy(other_item)
            for other_item in ConfigCache.get_named_transforms()
            if other_item != item
        ]

        config.clearNamedTransforms()

        for other_item in items:
            config.addNamedTransform(other_item)

    def _new_item(self, name: str) -> None:
        ocio.GetCurrentConfig().addNamedTransform(
            ocio.NamedTransform(name=name, forwardTransform=ocio.GroupTransform())
        )

    def _get_value(self, item: ocio.NamedTransform, column_desc: ColumnDesc) -> Any:
        # Get parameters
        if column_desc == self.NAME:
            return item.getName()
        elif column_desc == self.ALIASES:
            return list(item.getAliases())
        elif column_desc == self.FAMILY:
            return item.getFamily()
        elif column_desc == self.ENCODING:
            return item.getEncoding()
        elif column_desc == self.DESCRIPTION:
            return item.getDescription()
        elif column_desc == self.CATEGORIES:
            return list(item.getCategories())

        # Get transforms
        elif column_desc in (self.FORWARD_TRANSFORM, self.INVERSE_TRANSFORM):
            return item.getTransform(
                ocio.TRANSFORM_DIR_FORWARD
                if column_desc == self.FORWARD_TRANSFORM
                else ocio.TRANSFORM_DIR_INVERSE
            )

        # Invalid column
        return None

    def _set_value(
        self,
        item: ocio.NamedTransform,
        column_desc: ColumnDesc,
        value: Any,
        index: QtCore.QModelIndex,
    ) -> None:
        config = ocio.GetCurrentConfig()
        new_item = copy.deepcopy(item)
        prev_item_name = item.getName()

        # Update parameters
        if column_desc == self.NAME:
            new_item.setName(value)
        elif column_desc == self.ALIASES:
            new_item.clearAliases()
            for alias in value:
                new_item.addAlias(alias)
        elif column_desc == self.FAMILY:
            new_item.setFamily(value)
        elif column_desc == self.ENCODING:
            new_item.setEncoding(value)
        elif column_desc == self.DESCRIPTION:
            new_item.setDescription(value)
        elif column_desc == self.CATEGORIES:
            new_item.clearCategories()
            for category in value:
                new_item.addCategory(category)

        # Update transforms
        elif column_desc in (self.FORWARD_TRANSFORM, self.INVERSE_TRANSFORM):
            new_item.setTransform(
                value,
                ocio.TRANSFORM_DIR_FORWARD
                if column_desc == self.FORWARD_TRANSFORM
                else ocio.TRANSFORM_DIR_INVERSE,
            )

        # Preserve item order when replacing item due to name change, which requires
        # removing the old item to add the new.
        if column_desc == self.NAME:
            items = [
                copy.deepcopy(other_item)
                for other_item in ConfigCache.get_named_transforms()
            ]
            config.clearNamedTransforms()
            for other_item in items:
                if other_item.getName() == prev_item_name:
                    config.addNamedTransform(new_item)
                    self.item_renamed.emit(new_item.getName(), prev_item_name)
                else:
                    config.addNamedTransform(other_item)

        # Item order is preserved for all other changes
        else:
            config.addNamedTransform(new_item)

        # Broadcast transform or name changes to subscribers
        if column_desc in (self.NAME, self.FORWARD_TRANSFORM, self.INVERSE_TRANSFORM):
            item_name = new_item.getName()
            self._update_tf_subscribers(
                item_name, prev_item_name if prev_item_name != item_name else None
            )
