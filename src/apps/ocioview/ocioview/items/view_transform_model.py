# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import copy
from typing import Any, Optional, Union

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui

from ..config_cache import ConfigCache
from ..utils import get_enum_member, get_glyph_icon
from .config_item_model import ColumnDesc, BaseConfigItemModel
from .utils import get_scene_to_display_transform, get_display_to_scene_transform


class ViewTransformModel(BaseConfigItemModel):
    """
    Item model for editing view transforms in the current config.
    """

    REFERENCE_SPACE_TYPE = ColumnDesc(0, "Reference Space Type", int)
    NAME = ColumnDesc(1, "Name", str)
    FAMILY = ColumnDesc(2, "Family", str)
    DESCRIPTION = ColumnDesc(3, "Description", str)
    CATEGORIES = ColumnDesc(4, "Categories", list)
    TO_REFERENCE = ColumnDesc(5, "To Reference", ocio.Transform)
    FROM_REFERENCE = ColumnDesc(6, "From Reference", ocio.Transform)

    # fmt: off
    COLUMNS = sorted([
        REFERENCE_SPACE_TYPE, NAME, FAMILY, DESCRIPTION, CATEGORIES,
        TO_REFERENCE, FROM_REFERENCE,
    ], key=lambda s: s.column)
    # fmt: on

    __item_type__ = ocio.ViewTransform
    __icon_glyph__ = "ph.intersect"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        self._ref_space_icons = {
            ocio.REFERENCE_SPACE_SCENE: get_glyph_icon("ph.sun"),
            ocio.REFERENCE_SPACE_DISPLAY: get_glyph_icon("ph.monitor"),
        }

    def get_item_names(self) -> list[str]:
        return [item.getName() for item in self._get_items()]

    def get_item_transforms(
        self, item_name: str
    ) -> tuple[Optional[ocio.Transform], Optional[ocio.Transform]]:
        # Get view name from subscription item name
        item_name = self.extract_subscription_item_name(item_name)

        config = ocio.GetCurrentConfig()
        view_transform = config.getViewTransform(item_name)
        if view_transform is not None:
            return (
                get_scene_to_display_transform(view_transform),
                get_display_to_scene_transform(view_transform),
            )
        else:
            return None, None

    def _get_icon(
        self, item: ocio.ViewTransform, column_desc: ColumnDesc
    ) -> Optional[QtGui.QIcon]:
        if column_desc == self.NAME:
            return (
                self._get_subscription_icon(item, column_desc)
                or self._ref_space_icons[item.getReferenceSpaceType()]
            )
        else:
            return None

    def _get_bg_color(
        self, item: __item_type__, column_desc: ColumnDesc
    ) -> Optional[QtGui.QColor]:
        if column_desc == self.NAME:
            return self._get_subscription_color(item, column_desc)
        else:
            return None

    def _get_items(self, preserve: bool = False) -> list[ocio.ViewTransform]:
        if preserve:
            self._items = [
                copy.deepcopy(item) for item in ConfigCache.get_view_transforms()
            ]
            return self._items
        else:
            return ConfigCache.get_view_transforms()

    def _clear_items(self) -> None:
        ocio.GetCurrentConfig().clearViewTransforms()

    def _add_item(self, item: ocio.ViewTransform) -> None:
        ocio.GetCurrentConfig().addViewTransform(item)

    def _remove_item(self, item: ocio.ViewTransform) -> None:
        config = ocio.GetCurrentConfig()
        items = [
            copy.deepcopy(other_item)
            for other_item in config.getViewTransforms()
            if other_item != item
        ]

        config.clearViewTransforms()

        for other_item in items:
            config.addViewTransform(other_item)

    def _new_item(self, name: str) -> None:
        ocio.GetCurrentConfig().addViewTransform(
            ocio.ViewTransform(
                referenceSpace=ocio.REFERENCE_SPACE_SCENE,
                name=name,
                toReference=ocio.GroupTransform(),
                fromReference=ocio.GroupTransform(),
            )
        )

    def _get_value(self, item: ocio.ViewTransform, column_desc: ColumnDesc) -> Any:
        # Get parameters
        if column_desc == self.REFERENCE_SPACE_TYPE:
            return int(item.getReferenceSpaceType().value)
        elif column_desc == self.NAME:
            return item.getName()
        elif column_desc == self.FAMILY:
            return item.getFamily()
        elif column_desc == self.DESCRIPTION:
            return item.getDescription()
        elif column_desc == self.CATEGORIES:
            return list(item.getCategories())

        # Get transforms
        elif column_desc in (self.TO_REFERENCE, self.FROM_REFERENCE):
            return item.getTransform(
                ocio.VIEWTRANSFORM_DIR_TO_REFERENCE
                if column_desc == self.TO_REFERENCE
                else ocio.VIEWTRANSFORM_DIR_FROM_REFERENCE
            )

        # Invalid column
        return None

    def _set_value(
        self,
        item: ocio.ViewTransform,
        column_desc: ColumnDesc,
        value: Any,
        index: QtCore.QModelIndex,
    ) -> None:
        config = ocio.GetCurrentConfig()
        new_item = None
        prev_item_name = item.getName()

        # Changing reference space type requires constructing a new item
        if column_desc == self.REFERENCE_SPACE_TYPE:
            member = get_enum_member(ocio.ReferenceSpaceType, value)
            if member is not None:
                new_item = ocio.ViewTransform(
                    referenceSpace=member,
                    name=item.getName(),
                    family=item.getFamily(),
                    description=item.getDescription(),
                    toReference=item.getTransform(ocio.VIEWTRANSFORM_DIR_TO_REFERENCE),
                    fromReference=item.getTransform(
                        ocio.VIEWTRANSFORM_DIR_FROM_REFERENCE
                    ),
                    categories=list(item.getCategories()),
                )

        # Otherwise get an editable copy of the current item
        if new_item is None:
            new_item = copy.deepcopy(item)

        # Update parameters
        if column_desc == self.NAME:
            new_item.setName(value)
        elif column_desc == self.FAMILY:
            new_item.setFamily(value)
        elif column_desc == self.DESCRIPTION:
            new_item.setDescription(value)
        elif column_desc == self.CATEGORIES:
            new_item.clearCategories()
            for category in value:
                new_item.addCategory(category)

        # Update transforms
        elif column_desc in (self.TO_REFERENCE, self.FROM_REFERENCE):
            new_item.setTransform(
                value,
                ocio.VIEWTRANSFORM_DIR_TO_REFERENCE
                if column_desc == self.TO_REFERENCE
                else ocio.VIEWTRANSFORM_DIR_FROM_REFERENCE,
            )

        # Preserve item order when replacing item due to name or reference space
        # type change, which requires removing the old item to add the new.
        if column_desc in (self.REFERENCE_SPACE_TYPE, self.NAME):
            items = [
                copy.deepcopy(other_item) for other_item in config.getViewTransforms()
            ]
            config.clearViewTransforms()
            for other_item in items:
                if other_item.getName() == prev_item_name:
                    config.addViewTransform(new_item)
                    item_name = new_item.getName()
                    if item_name != prev_item_name:
                        self.item_renamed.emit(item_name, prev_item_name)
                else:
                    config.addViewTransform(other_item)

        # Item order is preserved for all other changes
        else:
            config.addViewTransform(new_item)

        # Broadcast transform or name changes to subscribers
        if column_desc in (self.NAME, self.TO_REFERENCE, self.FROM_REFERENCE):
            item_name = new_item.getName()
            self._update_tf_subscribers(
                item_name, prev_item_name if prev_item_name != item_name else None
            )
