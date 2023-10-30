# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from collections import defaultdict
from dataclasses import dataclass, field
from typing import Any, Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui

from ..config_cache import ConfigCache
from .config_item_model import ColumnDesc, BaseConfigItemModel
from .display_model import View, DisplayModel
from .utils import ViewType


@dataclass
class SharedView(View):
    """Individual shared view storage."""

    displays: set[str] = field(default_factory=set)


class SharedViewModel(BaseConfigItemModel):
    """
    Item model for editing shared views in the current config.
    """

    NAME = ColumnDesc(0, "View", str)
    COLOR_SPACE = ColumnDesc(1, "Color Space", str)
    VIEW_TRANSFORM = ColumnDesc(2, "View Transform", str)
    LOOKS = ColumnDesc(3, "Looks", str)
    RULE = ColumnDesc(4, "Rule", str)
    DESCRIPTION = ColumnDesc(5, "Description", str)

    # fmt: off
    COLUMNS = sorted([
        NAME, COLOR_SPACE, VIEW_TRANSFORM, LOOKS, RULE, DESCRIPTION
    ], key=lambda s: s.column)
    # fmt: on

    __item_type__ = SharedView
    __icon_glyph__ = "ph.share-network-bold"

    @classmethod
    def get_view_type_icon(cls, view_type: ViewType) -> QtGui.QIcon:
        return DisplayModel.get_view_type_icon(view_type)

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        ConfigCache.register_reset_callback(self._reset_cache)

    def get_item_names(self) -> list[str]:
        return [v.name for v in self._get_items()]

    def _reset_cache(self) -> None:
        self._items = []

    def _get_items(self, preserve: bool = False) -> list[SharedView]:
        if ConfigCache.validate() and self._items:
            return self._items

        config = ocio.GetCurrentConfig()

        shared_view_display_map = defaultdict(set)
        for display in ConfigCache.get_displays():
            for view in ConfigCache.get_views(display, view_type=ocio.VIEW_SHARED):
                shared_view_display_map[view].add(display)

        self._items = []

        for name in ConfigCache.get_shared_views():
            self._items.append(
                SharedView(
                    ViewType.VIEW_SHARED,
                    name,
                    config.getDisplayViewColorSpaceName("", name),
                    config.getDisplayViewTransformName("", name),
                    config.getDisplayViewLooks("", name),
                    config.getDisplayViewRule("", name),
                    config.getDisplayViewDescription("", name),
                    shared_view_display_map[name],
                )
            )

        return self._items

    def _clear_items(self) -> None:
        config = ocio.GetCurrentConfig()
        for item in self._get_items():
            self._remove_item(item, config=config)

    def _add_item(self, item: SharedView) -> None:
        config = ocio.GetCurrentConfig()
        config.addSharedView(
            item.name,
            item.view_transform,
            item.color_space,
            item.looks,
            item.rule,
            item.description,
        )
        for display in item.displays:
            config.addDisplaySharedView(display, item.name)

    def _remove_item(
        self, item: SharedView, config: Optional[ocio.Config] = None
    ) -> None:
        if config is None:
            config = ocio.GetCurrentConfig()

        # Remove reference from all displays
        for display in item.displays:
            config.removeDisplayView(display, item.name)

        # Remove shared view
        config.removeSharedView(item.name)

    def _new_item(self, name: str) -> None:
        view_transform = ConfigCache.get_default_view_transform_name()
        if not view_transform:
            view_transforms = ConfigCache.get_view_transforms()
            if view_transforms:
                view_transform = view_transforms[0]
        if not view_transform:
            self.warning_raised.emit(
                f"Could not create {self.item_type_label().lower()} because no view "
                f"transforms are defined."
            )
            return

        self._add_item(
            SharedView(
                ViewType.VIEW_SHARED,
                name,
                ocio.OCIO_VIEW_USE_DISPLAY_NAME,
                view_transform,
            )
        )

    def _get_value(self, item: SharedView, column_desc: ColumnDesc) -> Any:
        # Get parameters
        if column_desc == self.NAME:
            return item.name
        elif column_desc == self.COLOR_SPACE:
            return item.color_space
        elif column_desc == self.VIEW_TRANSFORM:
            return item.view_transform
        elif column_desc == self.LOOKS:
            return item.looks
        elif column_desc == self.RULE:
            return item.rule
        elif column_desc == self.DESCRIPTION:
            return item.description

        # Invalid column
        return None

    def _set_value(
        self,
        item: SharedView,
        column_desc: ColumnDesc,
        value: Any,
        index: QtCore.QModelIndex,
    ) -> None:
        item_names = self.get_item_names()
        if item.name not in item_names:
            return

        config = ocio.GetCurrentConfig()
        prev_item_name = item.name
        items = self._get_items()
        item_index = item_names.index(item.name)

        self._clear_items()

        # Update parameters
        if column_desc == self.COLOR_SPACE:
            is_valid = True
            if value != ocio.OCIO_VIEW_USE_DISPLAY_NAME:
                color_space = config.getColorSpace(value)
                if (
                    color_space is None
                    or color_space.getReferenceSpaceType()
                    != ocio.REFERENCE_SPACE_DISPLAY
                ):
                    is_valid = False

            if is_valid:
                items[item_index].color_space = value

        elif column_desc == self.VIEW_TRANSFORM:
            items[item_index].view_transform = value
        elif column_desc == self.NAME:
            items[item_index].name = value
        elif column_desc == self.LOOKS:
            items[item_index].looks = value
        elif column_desc == self.RULE:
            items[item_index].rule = value
        elif column_desc == self.DESCRIPTION:
            items[item_index].description = value

        for other_item in items:
            self._add_item(other_item)

        if item.name != prev_item_name:
            # Tell views to follow selection to new item
            self.item_added.emit(item.name)

            self.item_renamed.emit(item.name, prev_item_name)
