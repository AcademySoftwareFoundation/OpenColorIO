# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from dataclasses import dataclass, field
from typing import Any, Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore

from ..config_cache import ConfigCache
from ..utils import next_name
from .config_item_model import ColumnDesc, BaseConfigItemModel
from .utils import ViewType, get_view_type


@dataclass
class View:
    """Individual view storage."""

    type: ViewType
    name: str
    color_space: str
    view_transform: str
    looks: str = ""
    rule: str = ""
    description: str = ""


@dataclass
class Display:
    """Individual display storage."""

    name: str
    views: list[View] = field(default_factory=list)


class DisplayModel(BaseConfigItemModel):
    """
    Item model for editing displays in the current config. This model
    also tracks associated views, but is not responsible for editing
    views.
    """

    NAME = ColumnDesc(0, "Display", str)

    COLUMNS = [NAME]

    __item_type__ = Display
    __icon_glyph__ = "mdi6.monitor"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        ConfigCache.register_reset_callback(self._reset_cache)

    def get_item_names(self) -> list[str]:
        return [v.name for v in self._get_items()]

    def _reset_cache(self) -> None:
        self._items = []

    def _get_items(self, preserve: bool = False) -> list[Display]:
        if ConfigCache.validate() and self._items:
            return self._items

        config = ocio.GetCurrentConfig()

        self._items = []

        # Get views to preserve through display name changes
        for name in ConfigCache.get_displays():
            display = Display(name)

            # Display-defined views
            for view in ConfigCache.get_views(
                name, view_type=ocio.VIEW_DISPLAY_DEFINED
            ):
                display.views.append(
                    View(
                        get_view_type(name, view),
                        view,
                        config.getDisplayViewColorSpaceName(name, view),
                        config.getDisplayViewTransformName(name, view),
                        config.getDisplayViewLooks(name, view),
                        config.getDisplayViewRule(name, view),
                        config.getDisplayViewDescription(name, view),
                    )
                )

            # Shared views
            for view in ConfigCache.get_views(name, view_type=ocio.VIEW_SHARED):
                display.views.append(
                    View(
                        ViewType.VIEW_SHARED,
                        view,
                        config.getDisplayViewColorSpaceName("", view),
                        config.getDisplayViewTransformName("", view),
                        config.getDisplayViewLooks("", view),
                        config.getDisplayViewRule("", view),
                        config.getDisplayViewDescription("", view),
                    )
                )

            self._items.append(display)

        return self._items

    def _clear_items(self) -> None:
        # Remove all display and views. Shared views are preserved.
        config = ocio.GetCurrentConfig()
        config.clearDisplays()

    def _add_item(self, item: Display) -> None:
        config = ocio.GetCurrentConfig()
        for view in item.views:
            if view.type == ViewType.VIEW_SHARED:
                config.addDisplaySharedView(item.name, view.name)
            elif view.type == ViewType.VIEW_DISPLAY:
                config.addDisplayView(
                    item.name,
                    view.name,
                    view.view_transform,
                    view.color_space,
                    view.looks,
                    view.rule,
                    view.description,
                )
            else:  # VIEW_SCENE
                config.addDisplayView(
                    item.name, view.name, view.color_space, view.looks
                )

    def _remove_item(self, item: Display) -> None:
        # Remove all views from display. The display will be removed once it has no
        # associated views. Shared views will be disassociated, but preserved. Views
        # must be removed in reverse to preserve internal indices.
        config = ocio.GetCurrentConfig()
        for view in reversed(item.views):
            config.removeDisplayView(item.name, view.name)

    def _new_item(self, name: str) -> None:
        config = ocio.GetCurrentConfig()

        view_transform = ConfigCache.get_default_view_transform_name()
        if not view_transform:
            view_transforms = ConfigCache.get_view_transforms()
            if view_transforms:
                view_transform = view_transforms[0]

        color_space = None

        # A display can't exist without a view, so we'll add an initial view with it.
        # Prefer display-referred view if a view transform exists
        if view_transform:
            color_spaces = ConfigCache.get_color_space_names(
                ocio.SEARCH_REFERENCE_SPACE_DISPLAY
            )
            if color_spaces:
                color_space = color_spaces[0]

        if not color_space:
            color_space = ConfigCache.get_default_color_space_name()

        if color_space:
            # Generate unique view name
            views = ConfigCache.get_views()
            new_view = next_name("View_", views)

            # Add new display and view
            if view_transform:
                config.addDisplayView(
                    name,
                    new_view,
                    viewTransform=view_transform,
                    displayColorSpaceName=color_space,
                )
            else:
                config.addDisplayView(name, new_view, colorSpaceName=color_space)

    def _get_value(self, item: Display, column_desc: ColumnDesc) -> Any:
        # Get parameters
        if column_desc == self.NAME:
            return item.name

        # Invalid column
        return None

    def _set_value(
        self,
        item: Display,
        column_desc: ColumnDesc,
        value: Any,
        index: QtCore.QModelIndex,
    ) -> None:
        item_names = self.get_item_names()
        if item.name not in item_names:
            return

        prev_item_name = item.name
        items = self._get_items()
        item_index = item_names.index(item.name)

        # Update parameters
        if column_desc == self.NAME:
            is_valid = True
            config = ocio.GetCurrentConfig()
            for view in item.views:
                if (
                    view.type == ViewType.VIEW_SHARED
                    and view.color_space == ocio.OCIO_VIEW_USE_DISPLAY_NAME
                ):
                    new_display_color_space = config.getColorSpace(value)
                    if (
                        new_display_color_space is None
                        or new_display_color_space.getReferenceSpaceType()
                        != ocio.REFERENCE_SPACE_DISPLAY
                    ):
                        self.warning_raised.emit(
                            f"Display '{item.name}' has one or more shared views which "
                            f"derive their color space from the display name. No "
                            f"display color spaces named '{value}' exist. Please "
                            f"either choose a display name that matches a display "
                            f"color space name, or remove shared views from this "
                            f"display that utilize '{ocio.OCIO_VIEW_USE_DISPLAY_NAME}' "
                            f"for their color space."
                        )
                        is_valid = False
                        break

            if is_valid:
                items[item_index].name = value

        # Make sure local item instance matches item in items list
        item = items[item_index]

        if item.name != prev_item_name:
            # Rebuild display views with new name
            self._clear_items()
            for other_item in items:
                self._add_item(other_item)

            # Tell views to follow selection to new item
            self.item_added.emit(value)

            self.item_renamed.emit(item.name, prev_item_name)
