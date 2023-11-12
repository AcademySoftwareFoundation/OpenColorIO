# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import copy
from typing import Any, Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui

from ..config_cache import ConfigCache
from ..ref_space_manager import ReferenceSpaceManager
from ..utils import get_enum_member, get_glyph_icon
from .config_item_model import ColumnDesc, BaseConfigItemModel


class ColorSpaceModel(BaseConfigItemModel):
    """
    Item model for editing color spaces in the current config.
    """

    REFERENCE_SPACE_TYPE = ColumnDesc(0, "Reference Space Type", int)
    NAME = ColumnDesc(1, "Name", str)
    ALIASES = ColumnDesc(2, "Aliases", list)
    FAMILY = ColumnDesc(3, "Family", str)
    ENCODING = ColumnDesc(4, "Encoding", str)
    EQUALITY_GROUP = ColumnDesc(5, "Equality Group", str)
    DESCRIPTION = ColumnDesc(6, "Description", str)
    BIT_DEPTH = ColumnDesc(7, "Bit-Depth", int)
    IS_DATA = ColumnDesc(8, "Is Data", bool)
    ALLOCATION = ColumnDesc(9, "Allocation", int)
    ALLOCATION_VARS = ColumnDesc(10, "Allocation Vars", list)
    CATEGORIES = ColumnDesc(11, "Categories", list)
    TO_REFERENCE = ColumnDesc(12, "To Reference", ocio.Transform)
    FROM_REFERENCE = ColumnDesc(13, "From Reference", ocio.Transform)

    # fmt: off
    COLUMNS = sorted([
        REFERENCE_SPACE_TYPE, NAME, ALIASES, FAMILY, ENCODING, EQUALITY_GROUP,
        DESCRIPTION, BIT_DEPTH, IS_DATA, ALLOCATION, ALLOCATION_VARS,  CATEGORIES,
        TO_REFERENCE, FROM_REFERENCE,
    ], key=lambda s: s.column)
    # fmt: on

    __item_type__ = ocio.ColorSpace
    __icon_glyph__ = "ph.swap"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        self._items = ocio.ColorSpaceSet()

        self._ref_space_icons = {
            ocio.REFERENCE_SPACE_SCENE: get_glyph_icon("ph.sun"),
            ocio.REFERENCE_SPACE_DISPLAY: get_glyph_icon("ph.monitor"),
        }

        # Update on external config changes, in this case when a required reference
        # space is created.
        ReferenceSpaceManager.subscribe_to_reference_spaces(self.repaint)

    def get_item_names(self) -> list[str]:
        return [item.getName() for item in self._get_items()]

    def get_item_transforms(
        self, item_name: str
    ) -> tuple[Optional[ocio.Transform], Optional[ocio.Transform]]:
        # Get view name from subscription item name
        item_name = self.extract_subscription_item_name(item_name)

        ref_space_name = ReferenceSpaceManager.scene_reference_space().getName()
        return (
            ocio.ColorSpaceTransform(src=ref_space_name, dst=item_name),
            ocio.ColorSpaceTransform(src=item_name, dst=ref_space_name),
        )

    def _can_item_be_removed(self, item: ocio.ColorSpace) -> tuple[bool, str]:
        config = ocio.GetCurrentConfig()
        if config.isColorSpaceUsed(item.getName()):
            return (
                False,
                "is referenced by one or more transforms, roles, or looks in the "
                "config.",
            )
        else:
            return True, ""

    def _get_icon(
        self, item: ocio.ColorSpace, column_desc: ColumnDesc
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

    def _get_items(self, preserve: bool = False) -> list[ocio.ColorSpace]:
        if preserve:
            self._items = ConfigCache.get_color_spaces(as_set=True)
            return list(self._items.getColorSpaces())
        else:
            return ConfigCache.get_color_spaces()

    def _clear_items(self) -> None:
        ocio.GetCurrentConfig().clearColorSpaces()

    def _add_item(self, item: ocio.ColorSpace) -> None:
        ocio.GetCurrentConfig().addColorSpace(item)

    def _remove_item(self, item: ocio.ColorSpace) -> None:
        ocio.GetCurrentConfig().removeColorSpace(item.getName())

    def _new_item(self, name: str) -> None:
        ocio.GetCurrentConfig().addColorSpace(
            ocio.ColorSpace(referenceSpace=ocio.REFERENCE_SPACE_SCENE, name=name)
        )

    def _get_value(self, item: ocio.ColorSpace, column_desc: ColumnDesc) -> Any:
        # Get parameters
        if column_desc == self.REFERENCE_SPACE_TYPE:
            return int(item.getReferenceSpaceType().value)
        elif column_desc == self.NAME:
            return item.getName()
        elif column_desc == self.ALIASES:
            return list(item.getAliases())
        elif column_desc == self.FAMILY:
            return item.getFamily()
        elif column_desc == self.ENCODING:
            return item.getEncoding()
        elif column_desc == self.EQUALITY_GROUP:
            return item.getEqualityGroup()
        elif column_desc == self.DESCRIPTION:
            return item.getDescription()
        elif column_desc == self.BIT_DEPTH:
            return int(item.getBitDepth().value)
        elif column_desc == self.IS_DATA:
            return item.isData()
        elif column_desc == self.ALLOCATION:
            return int(item.getAllocation().value)
        elif column_desc == self.CATEGORIES:
            return list(item.getCategories())

        # Get allocation variables
        elif column_desc == self.ALLOCATION_VARS:
            # Make sure there are exactly three values, for compatibility with the
            # mapped value edit array.
            alloc_vars = item.getAllocationVars()
            num_alloc_vars = len(alloc_vars)
            if num_alloc_vars < 3:
                default_alloc_vars = [0.0, 1.0, 0.0]
                alloc_vars += [default_alloc_vars[i] for i in range(num_alloc_vars, 3)]
            elif num_alloc_vars > 3:
                alloc_vars = alloc_vars[:3]
            return alloc_vars

        # Get transforms
        elif column_desc in (self.TO_REFERENCE, self.FROM_REFERENCE):
            return item.getTransform(
                ocio.COLORSPACE_DIR_TO_REFERENCE
                if column_desc == self.TO_REFERENCE
                else ocio.COLORSPACE_DIR_FROM_REFERENCE
            )

        # Invalid column
        return None

    def _set_value(
        self,
        item: ocio.ColorSpace,
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
                new_item = ocio.ColorSpace(
                    referenceSpace=member,
                    name=item.getName(),
                    aliases=list(item.getAliases()),
                    family=item.getFamily(),
                    encoding=item.getEncoding(),
                    equalityGroup=item.getEqualityGroup(),
                    description=item.getDescription(),
                    bitDepth=item.getBitDepth(),
                    isData=item.isData(),
                    allocation=item.getAllocation(),
                    allocationVars=item.getAllocationVars(),
                    toReference=item.getTransform(ocio.COLORSPACE_DIR_TO_REFERENCE),
                    fromReference=item.getTransform(ocio.COLORSPACE_DIR_FROM_REFERENCE),
                    categories=list(item.getCategories()),
                )

        # Otherwise get an editable copy of the current item
        if new_item is None:
            new_item = copy.deepcopy(item)

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
        elif column_desc == self.EQUALITY_GROUP:
            new_item.setEqualityGroup(value)
        elif column_desc == self.DESCRIPTION:
            new_item.setDescription(value)
        elif column_desc == self.BIT_DEPTH:
            member = get_enum_member(ocio.BitDepth, value)
            if member is not None:
                new_item.setBitDepth(member)
        elif column_desc == self.IS_DATA:
            new_item.setIsData(value)
        elif column_desc == self.ALLOCATION:
            member = get_enum_member(ocio.Allocation, value)
            if member is not None:
                new_item.setAllocation(member)
        elif column_desc == self.ALLOCATION_VARS:
            new_item.setAllocationVars(value)
        elif column_desc == self.CATEGORIES:
            new_item.clearCategories()
            for category in value:
                new_item.addCategory(category)

        # Update transforms
        elif column_desc in (self.TO_REFERENCE, self.FROM_REFERENCE):
            new_item.setTransform(
                value,
                ocio.COLORSPACE_DIR_TO_REFERENCE
                if column_desc == self.TO_REFERENCE
                else ocio.COLORSPACE_DIR_FROM_REFERENCE,
            )

        # Preserve item order when replacing item due to name or reference space
        # type change, which requires removing the old item to add the new.
        if column_desc in (self.REFERENCE_SPACE_TYPE, self.NAME):
            items = ConfigCache.get_color_spaces(as_set=True)
            config.clearColorSpaces()
            for other_item in items.getColorSpaces():
                if other_item.getName() == prev_item_name:
                    config.addColorSpace(new_item)
                    item_name = new_item.getName()
                    if item_name != prev_item_name:
                        self.item_renamed.emit(item_name, prev_item_name)
                else:
                    config.addColorSpace(other_item)

        # Item order is preserved for all other changes
        else:
            config.addColorSpace(new_item)

        # Broadcast transform or name changes to subscribers
        if column_desc in (self.NAME, self.TO_REFERENCE, self.FROM_REFERENCE):
            item_name = new_item.getName()
            self._update_tf_subscribers(
                item_name, prev_item_name if prev_item_name != item_name else None
            )
