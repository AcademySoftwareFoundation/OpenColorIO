# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

import enum
from contextlib import contextmanager
from functools import partial
from typing import Callable, Optional, Union

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from ..config_cache import ConfigCache
from ..signal_router import SignalRouter
from ..utils import SignalsBlocked


class ComboBox(QtWidgets.QComboBox):
    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)
        self.setSizeAdjustPolicy(
            QtWidgets.QComboBox.AdjustToMinimumContentsLengthWithIcon
        )

    # DataWidgetMapper user property interface
    @QtCore.Property(str, user=True)
    def __data(self) -> str:
        return self.value()

    @__data.setter
    def __data(self, data: str) -> None:
        with SignalsBlocked(self):
            self.set_value(data)

    # Common public interface
    def value(self) -> str:
        return self.currentText()

    def set_value(self, value: str) -> None:
        self.setCurrentText(value)

    def reset(self) -> None:
        if self.isEditable():
            self.setEditText("")
        elif self.count():
            self.setCurrentIndex(0)


class EnumComboBox(ComboBox):
    """Combo box with an enum model."""

    def __init__(
        self,
        enum_type: enum.Enum,
        icons: Optional[dict[enum.Enum, QtGui.QIcon]] = None,
        parent: Optional[QtCore.QObject] = None,
    ):
        super().__init__(parent=parent)

        for name, member in enum_type.__members__.items():
            if icons is not None and member in icons:
                self.addItem(icons[member], name, userData=member)
            else:
                self.addItem(name, userData=member)

    # DataWidgetMapper user property interface
    @QtCore.Property(int, user=True)
    def __data(self) -> int:
        return self.currentIndex()

    @__data.setter
    def __data(self, data: int) -> None:
        with SignalsBlocked(self):
            self.setCurrentIndex(data)

    # Direct enum member access
    def member(self) -> enum.Enum:
        return self.currentData()

    def set_member(self, value: enum.Enum) -> None:
        self.setCurrentText(value.name)


class CallbackComboBox(ComboBox):
    """Combo box modeled around provided item callback(s)."""

    def __init__(
        self,
        get_items: Callable,
        get_default_item: Optional[Callable] = None,
        item_icon: Optional[QtGui.QIcon] = None,
        editable: bool = False,
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param get_items: Required callback which receives no
            parameters and returns a list of item strings, or a
            dictionary with item string keys and QIcon values, to add
            to combo box.
        :param get_default_item: Optional callback which receives no
            parameters and returns the default item string. If unset,
            the first item is the default.
        :param item_icon: Optionally provide one static icon for all
            items. Icons provided by 'get_items' take precedence.
        :param editable: Whether combo box is editable
        """
        super().__init__(parent=parent)

        self._get_items = get_items
        self._get_default_item = get_default_item
        self._item_icon = item_icon

        self.setEditable(editable)
        self.setInsertPolicy(QtWidgets.QComboBox.NoInsert)

        completer = self.completer()
        if completer is not None:
            completer.setCompletionMode(QtWidgets.QCompleter.PopupCompletion)

        # Initialize
        self.update_items()

    # DataWidgetMapper user property interface
    @QtCore.Property(str, user=True)
    def __data(self) -> str:
        return self.value()

    @__data.setter
    def __data(self, data: str) -> None:
        with SignalsBlocked(self):
            if self.findText(data) == -1:
                self.update_items()
            self.set_value(data)

    def update_items(self) -> str:
        """
        Call the provided callback(s) to update combo box items.

        :return: Current item string
        """
        # Get current state
        current_item = self.currentText()
        current_item_restored = False

        # Reload all items
        with SignalsBlocked(self):
            self.clear()
            items = self._get_items()
            if isinstance(items, dict):
                for item, icon in items.items():
                    if icon is None:
                        icon = self._item_icon
                    self.addItem(icon, item)
            else:
                if self._item_icon is not None:
                    for item in self._get_items():
                        self.addItem(self._item_icon, item)
                else:
                    self.addItems(self._get_items())

            # Restore original state
            index = self.findText(current_item)
            if index != -1:
                self.setCurrentIndex(index)
                current_item_restored = True

        if not current_item_restored and self._get_default_item is not None:
            self.setCurrentText(self._get_default_item())

        return self.currentText()

    def showPopup(self) -> None:
        """
        Reload items whenever the popup is shown for just-in-time
        model updates.
        """
        text = self.update_items()

        super().showPopup()

        # This selects the current item in the popup and must be called after the
        # popup is shown.
        items = self.model().findItems(text)
        if items:
            self.view().setCurrentIndex(items[0].index())

    def reset(self) -> None:
        super().reset()
        self.update_items()


class ColorSpaceComboBox(QtWidgets.QPushButton):
    """
    Combo box which maintains a menu of all active color spaces in the
    current config.
    """

    color_space_changed = QtCore.Signal()

    def __init__(
        self,
        reference_space_type: Optional[ocio.SearchReferenceSpaceType] = None,
        include_roles: bool = False,
        include_use_display_name: bool = False,
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param reference_space_type: Optional reference space type.
            Defaults to all reference spaces.
        :param include_roles: Whether to include a 'Roles' sub-menu
        :param include_use_display_name: Whether to include a special
            '<USE_DISPLAY_NAME>' item, used when creating shared views.
        """
        super().__init__(parent)
        self.setStyleSheet("text-align: left; padding-left: 4px;")
        self.setMinimumHeight(24)

        if reference_space_type is None:
            reference_space_type = ocio.SEARCH_REFERENCE_SPACE_ALL

        self._reference_space_type = reference_space_type
        self._include_roles = include_roles
        self._include_use_display_name = include_use_display_name
        self._config_cache_id = None
        self._menu = QtWidgets.QMenu()
        self._color_space_actions: dict[str, QtGui.QAction] = {}
        self._value: str | None = None

        # Initialize menu
        self.update_color_spaces()
        self._start_external_updates()

    # DataWidgetMapper user property interface
    @QtCore.Property(str, user=True)
    def __data(self) -> str:
        return self.color_space_name()

    @__data.setter
    def __data(self, data: str) -> None:
        with SignalsBlocked(self):
            self.set_color_space(data)

    def set_color_space(
        self, color_space_or_name: Union[ocio.ColorSpace, str]
    ) -> bool:
        """
        Set the selected color space.

        :param color_space_or_name: Color space object or name
        :return: Whether the color space was selected
        """
        # Handle special shared view case
        if color_space_or_name == ocio.OCIO_VIEW_USE_DISPLAY_NAME:
            if self._include_use_display_name:
                # Complete selection
                self._commit_value(ocio.OCIO_VIEW_USE_DISPLAY_NAME)
                return True
            else:
                return False

        # Detect argument type
        if isinstance(color_space_or_name, ocio.ColorSpace):
            color_space_name = color_space_or_name.getName()
        else:
            color_space_name = color_space_or_name

        # Verify color space
        config = ocio.GetCurrentConfig()
        color_space = config.getColorSpace(color_space_name)
        if color_space is not None:
            # Uncheck all color spaces
            for other_action in self._color_space_actions.values():
                other_action.setChecked(False)

            # Check selected color space
            action = self._color_space_actions.get(color_space_name)
            if action is None:
                return False
            else:
                action.setChecked(True)

            # Complete selection
            self._commit_value(color_space_name, action.text())
            return True
        else:
            return False

    def color_space(self) -> Union[ocio.ColorSpace, str, None]:
        """
        Get the selected color space.

        :return: Color space object, or None if no color space is
            selected. `OCIO_VIEW_USE_DISPLAY_NAME` may ne returned
            if 'include_use_display_name' was True on initialization.
        """
        # Handle special shared view case
        if (
            self._include_use_display_name
            and self._value == ocio.OCIO_VIEW_USE_DISPLAY_NAME
        ):
            return ocio.OCIO_VIEW_USE_DISPLAY_NAME

        # Lookup and return the color space instance
        config = ocio.GetCurrentConfig()
        if self._value:
            return config.getColorSpace(self._value)
        else:
            return None

    def color_space_name(self) -> str | None:
        """
        Get the selected color space name.

        :return: Color space name
        """
        # Handle special shared view case
        if (
            self._include_use_display_name
            and self._value == ocio.OCIO_VIEW_USE_DISPLAY_NAME
        ):
            return ocio.OCIO_VIEW_USE_DISPLAY_NAME

        config = ocio.GetCurrentConfig()

        # Is value a role?
        if config.hasRole(self._value):
            return self._value

        # Make sure value still references a color space
        color_space = self.color_space()
        if color_space is not None:
            return color_space.getName()
        else:
            return None

    def update_color_spaces(self) -> None:
        """Reload color spaces from the current config."""
        config_cache_id = ConfigCache.get_cache_id()
        if ConfigCache.validate() and config_cache_id == self._config_cache_id:
            return

        self._config_cache_id = config_cache_id

        # Preserve existing selection if possible
        current_name = self.color_space_name()
        current_name_available = False

        # Delete previous menu and its actions
        self._color_space_actions.clear()
        prev_menu = self._menu
        if prev_menu is not None:
            prev_menu.deleteLater()

        # Replace menu
        self._menu = QtWidgets.QMenu()
        self.setMenu(self._menu)

        # Add special shared view action
        if self._include_use_display_name:
            action = self._menu.addAction(
                ocio.OCIO_VIEW_USE_DISPLAY_NAME,
                partial(self.set_color_space, ocio.OCIO_VIEW_USE_DISPLAY_NAME),
            )
            action.setCheckable(True)
            self._color_space_actions[ocio.OCIO_VIEW_USE_DISPLAY_NAME] = action
            self._menu.addSeparator()

        # Configure color space menu helper
        config = ocio.GetCurrentConfig()

        menu_params = ocio.ColorSpaceMenuParameters(config)
        menu_params.setIncludeColorSpaces()
        menu_params.setSearchReferenceSpaceType(self._reference_space_type)
        menu_params.setIncludeRoles(self._include_roles)

        # Build menu hierarchy
        menu_helper = ocio.ColorSpaceMenuHelper(menu_params)

        for i in range(menu_helper.getNumColorSpaces()):
            name = menu_helper.getName(i)
            label = menu_helper.getUIName(i)
            family = menu_helper.getFamily(i)
            description = menu_helper.getDescription(i)

            if name == current_name:
                current_name_available = True

            if family == "Roles":
                self._menu.addSeparator()

            parent_menu = self._menu
            for level in menu_helper.getHierarchyLevels(i):
                child_menu = parent_menu.findChild(
                    QtWidgets.QMenu,
                    level,
                    options=QtCore.Qt.FindDirectChildrenOnly,
                )
                if child_menu is None:
                    child_menu = parent_menu.addMenu(level)
                    child_menu.setObjectName(level)
                parent_menu = child_menu

            # Add color space action
            action = parent_menu.addAction(
                label, partial(self.set_color_space, name)
            )
            action.setToolTip(description)
            action.setCheckable(True)
            self._color_space_actions[name] = action

        # Restore previous selection or select a reasonable default
        if current_name_available:
            self.set_color_space(current_name)
        else:
            default_name = ConfigCache.get_default_color_space_name()
            if default_name:
                self.set_color_space(default_name)
            else:
                self.setText("")

    def _commit_value(self, value: str, label: Optional[str] = None) -> None:
        """Commit color space value and broadcast to listeners."""
        with self._external_updates_paused():
            self._value = value
            self.setText(label or value)
            self.color_space_changed.emit()

    def _start_external_updates(self) -> None:
        """Start color space updates from external config changes."""
        signal_router = SignalRouter.get_instance()
        signal_router.config_reloaded.connect(self.update_color_spaces)
        signal_router.color_spaces_changed.connect(self.update_color_spaces)
        signal_router.roles_changed.connect(self.update_color_spaces)

    def _stop_external_updates(self) -> None:
        """Stop color space updates from external config changes."""
        signal_router = SignalRouter.get_instance()
        signal_router.config_reloaded.disconnect(self.update_color_spaces)
        signal_router.color_spaces_changed.disconnect(self.update_color_spaces)
        signal_router.roles_changed.disconnect(self.update_color_spaces)

    @contextmanager
    def _external_updates_paused(self) -> None:
        """
        Context manager to pause color space updates from external
        config changes within the enclosed scope.
        """
        self._stop_external_updates()
        yield
        self._start_external_updates()
