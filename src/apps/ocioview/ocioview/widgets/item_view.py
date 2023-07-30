# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Callable, Optional

from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import ICON_SIZE_BUTTON, ICON_SIZE_ITEM
from ..style import apply_top_tool_bar_style, apply_widget_with_top_tool_bar_style
from ..utils import get_glyph_icon
from .line_edit import LineEdit


class BaseItemView(QtWidgets.QFrame):
    """
    Abstract base class for adding a filter edit, add, remove, move up,
    and move down buttons to an item view.
    """

    items_changed = QtCore.Signal()
    current_row_changed = QtCore.Signal(int)

    DEFAULT_ITEM_FLAGS = (
        QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsSelectable
    )

    def __init__(
        self,
        item_view: QtWidgets.QListView,
        item_flags: QtCore.Qt.ItemFlags = DEFAULT_ITEM_FLAGS,
        item_icon: Optional[QtGui.QIcon] = None,
        items_constant: bool = False,
        items_movable: bool = True,
        get_presets: Optional[Callable] = None,
        presets_only: bool = False,
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param item_view: Item view to wrap
        :param item_flags: list item flags
        :param item_icon: Optional item icon
        :param items_constant: Optionally hide the add and remove
            buttons, for implementations where items are
            auto-populated. Note that preset support is dependent on
            this being False.
        :param items_movable: Optionally hide item movement buttons,
            for implementations where items are auto-sorted.
        :param get_presets: Optional callback which returns either a
            list of string presets, or a dictionary of string presets
            and corresponding item icons, that can be selected from an
            add button popup menu.
        :param presets_only: When True, only preset items may be added.
            Clicking the add button will present the preset menu
            instead of adding an item to the view.
        """
        super().__init__(parent=parent)

        self._item_flags = item_flags
        self._item_icon = item_icon
        self._items_constant = items_constant
        self._items_movable = items_movable
        self._has_presets = get_presets is not None
        self._get_presets = get_presets or (lambda: [])
        self._presets_only = presets_only

        self.setFrameShape(QtWidgets.QFrame.StyledPanel)
        self.setObjectName("item_view")
        apply_widget_with_top_tool_bar_style(self)

        # Widgets
        self.filter_edit = LineEdit()
        self.filter_edit.setClearButtonEnabled(True)
        self.filter_edit.setPlaceholderText("filter")
        self.filter_edit.textChanged.connect(self._on_filter_text_changed)

        if not self._items_constant:
            # Add button preset menu
            self.preset_menu = QtWidgets.QMenu(self)
            self.preset_menu.aboutToShow.connect(self._on_preset_menu_requested)
            self.preset_menu.triggered.connect(self._on_preset_triggered)

            self.add_button = QtWidgets.QToolButton(self)
            self.add_button.setIconSize(ICON_SIZE_BUTTON)
            self.add_button.setIcon(get_glyph_icon("ph.plus"))
            self.add_button.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
            self.add_button.released.connect(self._on_add_button_released)
            if self._has_presets:
                self.add_button.setMenu(self.preset_menu)
                if self._presets_only:
                    self.add_button.setPopupMode(QtWidgets.QToolButton.InstantPopup)
                    self.add_button.setToolButtonStyle(
                        QtCore.Qt.ToolButtonTextBesideIcon
                    )
                else:
                    self.add_button.setPopupMode(QtWidgets.QToolButton.MenuButtonPopup)

            self.remove_button = QtWidgets.QToolButton(self)
            self.remove_button.setIconSize(ICON_SIZE_BUTTON)
            self.remove_button.setIcon(get_glyph_icon("ph.minus"))
            self.remove_button.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
            self.remove_button.released.connect(self._on_remove_button_released)

        if self._items_movable:
            self.move_up_button = QtWidgets.QToolButton(self)
            self.move_up_button.setIconSize(ICON_SIZE_BUTTON)
            self.move_up_button.setIcon(get_glyph_icon("ph.arrow-up"))
            self.move_up_button.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
            self.move_up_button.released.connect(self._on_move_up_button_released)

            self.move_down_button = QtWidgets.QToolButton(self)
            self.move_down_button.setIconSize(ICON_SIZE_BUTTON)
            self.move_down_button.setIcon(get_glyph_icon("ph.arrow-down"))
            self.move_down_button.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
            self.move_down_button.released.connect(self._on_move_down_button_released)

        self.view = item_view
        self.view.setIconSize(ICON_SIZE_ITEM)

        # Layout
        tool_bar = QtWidgets.QToolBar()
        tool_bar.setStyleSheet(
            "QToolButton::menu-indicator {"
            "    subcontrol-position: center right;"
            "    right: 4px;"
            "}"
        )
        tool_bar.setContentsMargins(0, 0, 0, 0)
        tool_bar.setIconSize(ICON_SIZE_ITEM)
        tool_bar.addWidget(self.filter_edit)
        tool_bar.addWidget(QtWidgets.QLabel(""))
        if not self._items_constant:
            tool_bar.addWidget(self.add_button)
            tool_bar.addWidget(self.remove_button)
        if self._items_movable:
            tool_bar.addWidget(self.move_up_button)
            tool_bar.addWidget(self.move_down_button)

        tool_bar_layout = QtWidgets.QVBoxLayout()
        tool_bar_layout.setContentsMargins(0, 0, 0, 0)
        tool_bar_layout.addWidget(tool_bar)

        tool_bar_frame = QtWidgets.QFrame()
        tool_bar_frame.setFrameShape(QtWidgets.QFrame.StyledPanel)
        tool_bar_frame.setObjectName("item_view__tool_bar_frame")
        apply_top_tool_bar_style(tool_bar_frame)
        tool_bar_frame.setLayout(tool_bar_layout)

        layout = QtWidgets.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(1)
        layout.addWidget(tool_bar_frame)
        layout.addWidget(self.view)

        self.setLayout(layout)

    def reset(self) -> None:
        self.clear()

    def clear(self) -> None:
        """Remove all items from list."""
        raise NotImplementedError

    def items(self) -> list[str]:
        """
        :return: list of item names
        """
        raise NotImplementedError

    def set_current_item(self, text: str) -> tuple[bool, int]:
        """
        :param text: Make the named item the current item
        :return: Whether the requested item was selected, and the
            current row after any changes
        """
        raise NotImplementedError

    def add_item(self, text: Optional[str] = None) -> None:
        """
        Create a new list item.

        :param text: Optional item name
        """
        raise NotImplementedError

    def remove_item(self, text: str) -> None:
        """
        :param text: Name of list item to remove
        """
        raise NotImplementedError

    @QtCore.Slot(str)
    def _on_filter_text_changed(self, text: str) -> None:
        """
        Subclasses must implement list filtering behavior, hiding items
        which don't contain the provided search term.

        :param text: Filter search term
        """
        raise NotImplementedError

    def _on_add_button_released(self) -> None:
        """
        Subclasses must implement behavior which results from the
        widget's add button being clicked.
        """
        raise NotImplementedError

    def _on_remove_button_released(self) -> None:
        """
        Subclasses must implement behavior which results from the
        widget's remove button being clicked.
        """
        raise NotImplementedError

    def _on_move_up_button_released(self) -> None:
        """
        Subclasses must implement behavior which results from the
        widget's move up button being clicked.
        """
        raise NotImplementedError

    def _on_move_down_button_released(self) -> None:
        """
        Subclasses must implement behavior which results from the
        widget's move down button being clicked.
        """
        raise NotImplementedError

    def _on_preset_menu_requested(self) -> None:
        """Repopulate preset menu from callback."""
        self.preset_menu.clear()

        presets = self._get_presets()
        if isinstance(presets, dict):
            for preset, item_icon in presets.items():
                self.preset_menu.addAction(item_icon, preset)
        else:
            for preset in presets:
                if self._item_icon:
                    self.preset_menu.addAction(self._item_icon, preset)
                else:
                    self.preset_menu.addAction(preset)

    def _on_preset_triggered(self, action: QtGui.QAction) -> None:
        """Add a new item from the triggered preset."""
        self.add_item(action.text())
