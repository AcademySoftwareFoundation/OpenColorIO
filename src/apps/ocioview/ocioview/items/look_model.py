# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import copy
from typing import Any, Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui

from ..config_cache import ConfigCache
from ..ref_space_manager import ReferenceSpaceManager
from .config_item_model import ColumnDesc, BaseConfigItemModel


class LookModel(BaseConfigItemModel):
    """
    Item model for editing looks in the current config.
    """

    NAME = ColumnDesc(0, "Name", str)
    PROCESS_SPACE = ColumnDesc(1, "Process Space", str)
    TRANSFORM = ColumnDesc(2, "Transform", ocio.Transform)
    INVERSE_TRANSFORM = ColumnDesc(3, "Inverse Transform", ocio.Transform)
    DESCRIPTION = ColumnDesc(4, "Description", str)

    COLUMNS = sorted(
        [NAME, PROCESS_SPACE, TRANSFORM, INVERSE_TRANSFORM, DESCRIPTION],
        key=lambda s: s.column,
    )

    __item_type__ = ocio.Look
    __icon_glyph__ = "ri.clapperboard-line"

    def get_item_names(self) -> list[str]:
        return [item.getName() for item in self._get_items()]

    def get_item_transforms(
        self, item_name: str
    ) -> tuple[Optional[ocio.Transform], Optional[ocio.Transform]]:
        # Get view name from subscription item name
        item_name = self.extract_subscription_item_name(item_name)

        scene_ref_name = ReferenceSpaceManager.scene_reference_space().getName()
        return (
            ocio.LookTransform(
                src=scene_ref_name,
                dst=scene_ref_name,
                looks=item_name,
                direction=ocio.TRANSFORM_DIR_FORWARD,
            ),
            ocio.LookTransform(
                src=scene_ref_name,
                dst=scene_ref_name,
                looks=item_name,
                direction=ocio.TRANSFORM_DIR_INVERSE,
            ),
        )

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

    def _get_items(self, preserve: bool = False) -> list[ocio.Look]:
        # TODO: Revert to using ConfigCache following fix of issue:
        #       https://github.com/AcademySoftwareFoundation/OpenColorIO/issues/1817
        config = ocio.GetCurrentConfig()
        if preserve:
            # self._items = [copy.deepcopy(item) for item in ConfigCache.get_looks()]
            self._items = [copy.deepcopy(item) for item in config.getLooks()]
            return self._items
        else:
            # return ConfigCache.get_looks()
            return config.getLooks()

    def _clear_items(self) -> None:
        ocio.GetCurrentConfig().clearLooks()

    def _add_item(self, item: ocio.Look) -> None:
        ocio.GetCurrentConfig().addLook(item)

    def _remove_item(self, item: ocio.Look) -> None:
        config = ocio.GetCurrentConfig()
        items = [
            copy.deepcopy(other_item)
            for other_item in ConfigCache.get_looks()
            if other_item != item
        ]

        config.clearLooks()

        for other_item in items:
            config.addLook(other_item)

    def _new_item(self, name: str) -> None:
        config = ocio.GetCurrentConfig()
        color_space = ConfigCache.get_default_color_space_name()
        if not color_space:
            color_spaces = ConfigCache.get_color_space_names()
            if color_spaces:
                color_space = color_spaces[0]
        if color_space:
            config.addLook(ocio.Look(name=name, processSpace=color_space))
        else:
            config.addLook(ocio.Look(name=name))

    def _get_value(self, item: ocio.Look, column_desc: ColumnDesc) -> Any:
        config = ocio.GetCurrentConfig()

        # Get parameters
        if column_desc == self.NAME:
            return item.getName()
        elif column_desc == self.DESCRIPTION:
            return item.getDescription()

        # Process space (color space name)
        elif column_desc == self.PROCESS_SPACE:
            process_space = item.getProcessSpace()
            if not process_space:

                # Process space is unset; find a reasonable default. Start with the most
                # common roles for shot grades or ACES LMTs.
                for role in (ocio.ROLE_COLOR_TIMING, ocio.ROLE_INTERCHANGE_SCENE):
                    process_space = config.getCanonicalName(role)
                    if process_space:
                        break

                if not process_space:
                    # Next look for any log-encoded color space. This probably isn't the
                    # right choice, but will start in the right direction.
                    for color_space in config.getColorSpaces(
                        ocio.SEARCH_REFERENCE_SPACE_SCENE, ocio.COLORSPACE_ALL
                    ):
                        if color_space.getEncoding() == "log":
                            process_space = color_space.getName()
                            break

                if not process_space:
                    # Lastly, get the first color space. Something is better than
                    # nothing.
                    color_space_name_iter = config.getColorSpaceNames(
                        ocio.SEARCH_REFERENCE_SPACE_SCENE, ocio.COLORSPACE_ALL
                    )
                    try:
                        process_space = next(color_space_name_iter)
                    except StopIteration:
                        pass

            return process_space or ""

        # Get transforms
        elif column_desc == self.TRANSFORM:
            return item.getTransform()
        elif column_desc == self.INVERSE_TRANSFORM:
            return item.getInverseTransform()

        # Invalid column
        return None

    def _set_value(
        self,
        item: ocio.Look,
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
        elif column_desc == self.PROCESS_SPACE:
            new_item.setProcessSpace(value)
        elif column_desc == self.DESCRIPTION:
            new_item.setDescription(value)

        # Update transforms
        if column_desc == self.TRANSFORM:
            new_item.setTransform(value)
        elif column_desc == self.INVERSE_TRANSFORM:
            new_item.setInverseTransform(value)

        # Preserve item order when replacing item due to name change, which requires
        # removing the old item to add the new.
        if column_desc == self.NAME:
            items = [copy.deepcopy(other_item) for other_item in config.getLooks()]
            config.clearLooks()
            for other_look in items:
                if other_look.getName() == prev_item_name:
                    config.addLook(new_item)
                    self.item_renamed.emit(new_item.getName(), prev_item_name)
                else:
                    config.addLook(other_look)

        # Item order is preserved for all other changes
        else:
            config.addLook(new_item)

        # Broadcast transform or name changes to subscribers
        if column_desc in (self.NAME, self.TRANSFORM, self.INVERSE_TRANSFORM):
            item_name = new_item.getName()
            self._update_tf_subscribers(
                item_name, prev_item_name if prev_item_name != item_name else None
            )
