# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Any, Optional, Union

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui

from ..config_cache import ConfigCache
from ..ref_space_manager import ReferenceSpaceManager
from ..undo import ConfigSnapshotUndoCommand
from ..utils import get_glyph_icon, next_name
from .config_item_model import ColumnDesc, BaseConfigItemModel
from .display_model import View
from .utils import ViewType, get_view_type


class ViewModel(BaseConfigItemModel):
    """
    Item model for editing display-defined views in the current config.
    """

    VIEW_TYPE = ColumnDesc(0, "View Type", str)
    NAME = ColumnDesc(1, "View", str)
    COLOR_SPACE = ColumnDesc(2, "Color Space", str)
    VIEW_TRANSFORM = ColumnDesc(3, "View Transform", str)
    LOOKS = ColumnDesc(4, "Looks", str)
    RULE = ColumnDesc(5, "Rule", str)
    DESCRIPTION = ColumnDesc(6, "Description", str)

    # fmt: off
    COLUMNS = sorted([
        VIEW_TYPE, NAME, COLOR_SPACE, VIEW_TRANSFORM, LOOKS, RULE, DESCRIPTION
    ], key=lambda s: s.column)
    # fmt: on

    __item_type__ = View
    __icon_glyph__ = "mdi6.monitor-eye"

    @classmethod
    def get_view_type_icon(cls, view_type: ViewType) -> QtGui.QIcon:
        glyph_names = {
            ViewType.VIEW_SHARED: "ph.share-network-bold",
            ViewType.VIEW_DISPLAY: "mdi6.eye-outline",
            ViewType.VIEW_SCENE: "mdi6.eye",
        }
        return get_glyph_icon(glyph_names[view_type])

    @classmethod
    def has_presets(cls) -> bool:
        return True

    @classmethod
    def requires_presets(cls) -> bool:
        return True

    @classmethod
    def get_presets(cls) -> Optional[Union[list[str], dict[str, QtGui.QIcon]]]:
        presets = {
            ViewType.VIEW_DISPLAY: cls.get_view_type_icon(ViewType.VIEW_DISPLAY),
            ViewType.VIEW_SCENE: cls.get_view_type_icon(ViewType.VIEW_SCENE),
        }
        for view in ConfigCache.get_shared_views():
            presets[view] = cls.get_view_type_icon(ViewType.VIEW_SHARED)

        return presets

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        self._display = None

        self._view_type_icons = {
            ViewType.VIEW_SHARED: self.get_view_type_icon(ViewType.VIEW_SHARED),
            ViewType.VIEW_DISPLAY: self.get_view_type_icon(ViewType.VIEW_DISPLAY),
            ViewType.VIEW_SCENE: self.get_view_type_icon(ViewType.VIEW_SCENE),
        }

        ConfigCache.register_reset_callback(self._reset_cache)

    def set_display(self, display: str) -> None:
        """
        :param display: Display to model views for
        """
        self.beginResetModel()

        self._display = display
        self._reset_cache()

        self.endResetModel()

    def add_preset(self, preset_name: str) -> int:
        if not self._display:
            return -1

        item = None

        # Scene-referred view
        if preset_name == ViewType.VIEW_SCENE.value:
            color_spaces = ConfigCache.get_color_space_names(
                ocio.SEARCH_REFERENCE_SPACE_SCENE
            )
            if color_spaces:
                color_space = color_spaces[0]
                views = ConfigCache.get_views(view_type=ocio.VIEW_DISPLAY_DEFINED)

                item = View(
                    ViewType.VIEW_SCENE, next_name("View_", views), color_space, ""
                )
            else:
                self.warning_raised.emit(
                    f"Could not create {ViewType.VIEW_SCENE.value.lower()} because no "
                    f"color spaces with a scene reference type are defined."
                )

        # Display-referred view
        elif preset_name == ViewType.VIEW_DISPLAY.value:
            view_transform = ConfigCache.get_default_view_transform_name()
            if not view_transform:
                view_transforms = ConfigCache.get_view_transforms()
                if view_transforms:
                    view_transform = view_transforms[0]

            if view_transform:
                color_spaces = ConfigCache.get_color_space_names(
                    ocio.SEARCH_REFERENCE_SPACE_DISPLAY
                )
                if color_spaces:
                    color_space = color_spaces[0]
                    views = ConfigCache.get_views(view_type=ocio.VIEW_DISPLAY_DEFINED)

                    item = View(
                        ViewType.VIEW_DISPLAY,
                        next_name("View_", views),
                        color_space,
                        view_transform,
                    )
                else:
                    self.warning_raised.emit(
                        f"Could not create {ViewType.VIEW_DISPLAY.value.lower()} "
                        f"because no color spaces with a display reference type are "
                        f"defined."
                    )
            else:
                self.warning_raised.emit(
                    f"Could not create {ViewType.VIEW_DISPLAY.value.lower()} because "
                    f"no view transforms are defined."
                )

        # Shared view, which always follow display-defined views, as stored in the
        # config YAML data.
        else:
            item = View(ViewType.VIEW_SHARED, preset_name, "", "")

        # Append new view to display
        row = -1

        if item is not None:
            with ConfigSnapshotUndoCommand(
                f"Add {self.item_type_label()}", model=self, item_name=item.name
            ):
                self._add_item(item)
                row = self.get_item_names().index(item.name)

                self.beginInsertRows(self.NULL_INDEX, row, row)
                self.endInsertRows()
                self.item_added.emit(item.name)

        return row

    def move_item_up(self, item_name: str) -> bool:
        item_names = self.get_item_names()
        if item_name not in item_names:
            return False

        src_row = item_names.index(item_name)

        items = self._get_items()
        item = items[src_row]

        if src_row > 0:
            # Display-defined and shared views are kept separate, so we determine move
            # capability only relative to similar view types (with scene and
            # display-referred views being interchangeable).
            prev_item = items[src_row - 1]
            if (
                item.type != ViewType.VIEW_SHARED
                and prev_item.type != ViewType.VIEW_SHARED
            ) or (
                item.type == ViewType.VIEW_SHARED
                and prev_item.type == ViewType.VIEW_SHARED
            ):
                dst_row = src_row - 1
                return self.moveRows(
                    self.NULL_INDEX, src_row, 1, self.NULL_INDEX, dst_row
                )

        return False

    def move_item_down(self, item_name: str) -> bool:
        item_names = self.get_item_names()
        if item_name not in item_names:
            return False

        src_row = item_names.index(item_name)

        items = self._get_items()
        item = items[src_row]

        if src_row < len(items) - 1:
            # Display-defined and shared views are kept separate, so we determine move
            # capability only relative to similar view types (with scene and
            # display-referred views being interchangeable).
            next_item = items[src_row + 1]
            if (
                item.type != ViewType.VIEW_SHARED
                and next_item.type != ViewType.VIEW_SHARED
            ) or (
                item.type == ViewType.VIEW_SHARED
                and next_item.type == ViewType.VIEW_SHARED
            ):
                dst_row = src_row + 1
                return self.moveRows(
                    self.NULL_INDEX, src_row, 1, self.NULL_INDEX, dst_row
                )

        return False

    def get_item_names(self) -> list[str]:
        return [v.name for v in self._get_items()]

    def get_item(self, index: QtCore.QModelIndex) -> Optional[tuple[str, str]]:
        if self._display:
            items = self._get_items()
            row = index.row()
            if row < len(items):
                return self._display, items[row].name
        return None

    def get_item_transforms(
        self, item_name: str
    ) -> tuple[Optional[ocio.Transform], Optional[ocio.Transform]]:

        if self._display is not None:
            # Get view name from subscription item name
            item_name = self.extract_subscription_item_name(item_name)

            scene_ref_name = ReferenceSpaceManager.scene_reference_space().getName()
            return (
                ocio.DisplayViewTransform(
                    src=scene_ref_name,
                    display=self._display,
                    view=item_name,
                    direction=ocio.TRANSFORM_DIR_FORWARD,
                ),
                ocio.DisplayViewTransform(
                    src=scene_ref_name,
                    display=self._display,
                    view=item_name,
                    direction=ocio.TRANSFORM_DIR_INVERSE,
                ),
            )
        else:
            return None, None

    def format_subscription_item_name(
        self,
        item_name_or_index: Union[str, QtCore.QModelIndex],
        display: Optional[str] = None,
        **kwargs,
    ) -> Optional[str]:
        item_name = super().format_subscription_item_name(item_name_or_index)
        if item_name and (display or self._display):
            return f"{display or self._display}/{item_name}"
        else:
            return item_name

    def extract_subscription_item_name(self, subscription_item_name: str) -> str:
        item_name = super().extract_subscription_item_name(subscription_item_name)
        if self._display and item_name.startswith(self._display + "/"):
            item_name = item_name[len(self._display) + 1 :]
        return item_name

    def _get_undo_command_text(
        self, index: QtCore.QModelIndex, column_desc: ColumnDesc
    ) -> str:
        text = super()._get_undo_command_text(index, column_desc)
        if text:
            # Insert display name before view
            item_name = self.get_item_name(index)
            text = text.replace(
                f"({item_name})", f"({self.format_subscription_item_name(item_name)})"
            )
        return text

    def _get_icon(self, item: View, column_desc: ColumnDesc) -> Optional[QtGui.QIcon]:
        if column_desc == self.NAME:
            return (
                self._get_subscription_icon(item, column_desc)
                or self._view_type_icons[item.type]
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

    def _get_placeholder_view(self) -> View:
        """
        Get a placeholder view to keep a display alive while modifying
        (removing and restoring, in order) its actual views.

        :return: View instance
        """
        color_space = ConfigCache.get_default_color_space_name()
        if not color_space:
            color_spaces = ConfigCache.get_color_space_names()
            if color_spaces:
                color_space = color_spaces[0]
            else:
                # Add a color space so a view can exist
                color_space = "Raw"
                config = ocio.GetCurrentConfig()
                config.addColorSpace(
                    ocio.ColorSpace(
                        ocio.REFERENCE_SPACE_SCENE,
                        color_space,
                        bitDepth=ocio.BIT_DEPTH_F32,
                        isData=True,
                    )
                )

        return View(ViewType.VIEW_SCENE, "_", color_space, "")

    def _reset_cache(self) -> None:
        self._items = []

    def _get_items(self, preserve: bool = False) -> list[View]:
        if not self._display:
            return []

        if ConfigCache.validate() and self._items:
            return self._items

        config = ocio.GetCurrentConfig()

        self._items = []

        # Display views
        for name in config.getViews(ocio.VIEW_DISPLAY_DEFINED, self._display):
            self._items.append(
                View(
                    get_view_type(self._display, name),
                    name,
                    config.getDisplayViewColorSpaceName(self._display, name),
                    config.getDisplayViewTransformName(self._display, name),
                    config.getDisplayViewLooks(self._display, name),
                    config.getDisplayViewRule(self._display, name),
                    config.getDisplayViewDescription(self._display, name),
                )
            )

        # Shared views
        for name in config.getViews(ocio.VIEW_SHARED, self._display):
            self._items.append(
                View(
                    ViewType.VIEW_SHARED,
                    name,
                    config.getDisplayViewColorSpaceName("", name),
                    config.getDisplayViewTransformName("", name),
                    config.getDisplayViewLooks("", name),
                    config.getDisplayViewRule("", name),
                    config.getDisplayViewDescription("", name),
                )
            )

        return self._items

    def _clear_items(self) -> None:
        if self._display:
            config = ocio.GetCurrentConfig()

            # Insert placeholder view to keep display alive
            placeholder_view = self._get_placeholder_view()
            config.addDisplayView(
                self._display, placeholder_view.name, placeholder_view.color_space
            )

            # Views must be removed in reverse to preserve internal indices
            for view in reversed(ConfigCache.get_views(self._display)):
                if view != placeholder_view.name:
                    config.removeDisplayView(self._display, view)

    def _add_item(self, item: View) -> None:
        if self._display:
            config = ocio.GetCurrentConfig()
            try:
                if item.type == ViewType.VIEW_SHARED:
                    config.addDisplaySharedView(self._display, item.name)
                elif item.type == ViewType.VIEW_DISPLAY:
                    config.addDisplayView(
                        self._display,
                        item.name,
                        item.view_transform,
                        item.color_space,
                        item.looks,
                        item.rule,
                        item.description,
                    )
                else:  # ViewType.VIEW_SCENE
                    config.addDisplayView(
                        self._display, item.name, item.color_space, item.looks
                    )
            except ocio.Exception as e:
                self.warning_raised.emit(str(e))

            # Remove placeholder view, if present
            placeholder_view = self._get_placeholder_view()
            if placeholder_view.name in ConfigCache.get_views(
                self._display, view_type=ocio.VIEW_DISPLAY_DEFINED
            ):
                config.removeDisplayView(self._display, placeholder_view.name)

    def _remove_item(self, item: View) -> None:
        if self._display:
            config = ocio.GetCurrentConfig()
            config.removeDisplayView(self._display, item.name)

    def _new_item(self, name: str) -> None:
        # New items added through presets only
        pass

    def _get_value(self, item: View, column_desc: ColumnDesc) -> Any:
        # Get parameters
        if column_desc == self.VIEW_TYPE:
            return item.type
        elif column_desc == self.NAME:
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
        self, item: View, column_desc: ColumnDesc, value: Any, index: QtCore.QModelIndex
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
            color_space = config.getColorSpace(value)
            if color_space:
                if (
                    item.view_transform
                    and (
                        color_space.getReferenceSpaceType()
                        == ocio.REFERENCE_SPACE_DISPLAY
                    )
                ) or (
                    not item.view_transform
                    and color_space.getReferenceSpaceType()
                    == ocio.REFERENCE_SPACE_SCENE
                ):
                    items[item_index].color_space = value

        elif column_desc == self.VIEW_TRANSFORM:
            color_space = config.getColorSpace(item.color_space)
            if color_space and (
                color_space.getReferenceSpaceType() == ocio.REFERENCE_SPACE_DISPLAY
            ):
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

        # Make sure local item instance matches item in items list
        item = items[item_index]

        if item.name != prev_item_name:
            # Tell views to follow selection to new item
            self.item_added.emit(item.name)

            self.item_renamed.emit(item.name, prev_item_name)

        # Broadcast transform or name changes to subscribers
        if column_desc in (
            self.NAME,
            self.COLOR_SPACE,
            self.VIEW_TRANSFORM,
            self.LOOKS,
        ):
            self._update_tf_subscribers(
                item.name,
                prev_item_name if prev_item_name != item.name else None,
            )
