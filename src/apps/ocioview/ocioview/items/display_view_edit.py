# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from PySide6 import QtCore, QtGui, QtWidgets

from ..utils import get_glyph_icon
from .active_display_view_edit import ActiveDisplayViewEdit
from .shared_view_edit import SharedViewEdit
from .utils import adapt_splitter_sizes
from .view_edit import ViewEdit


class DisplayViewEdit(QtWidgets.QWidget):
    """
    Widget for editing all displays and views in the current config.
    """

    @classmethod
    def item_type_icon(cls) -> QtGui.QIcon:
        """
        :return: Item type icon
        """
        return get_glyph_icon("mdi6.monitor-eye")

    @classmethod
    def item_type_label(cls, plural: bool = False) -> str:
        """
        :param plural: Whether label should be plural
        :return: Friendly type name
        """
        return ViewEdit.item_type_label(plural=plural)

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        self._prev_index = 0

        # Widgets
        self.view_edit = ViewEdit()
        self.shared_view_edit = SharedViewEdit()
        self.active_display_view_edit = ActiveDisplayViewEdit()

        # Layout
        self.tabs = QtWidgets.QTabWidget()
        self.tabs.addTab(
            self.view_edit,
            self.view_edit.item_type_icon(),
            self.view_edit.item_type_label(plural=True),
        )
        self.tabs.addTab(
            self.shared_view_edit,
            self.shared_view_edit.item_type_icon(),
            self.shared_view_edit.item_type_label(plural=True),
        )
        self.tabs.addTab(
            self.active_display_view_edit,
            self.active_display_view_edit.item_type_icon(),
            self.active_display_view_edit.item_type_label(plural=True),
        )
        self.tabs.currentChanged.connect(self._on_current_changed)

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.tabs)
        self.setLayout(layout)

        # Connections
        self.view_edit.shared_view_selection_requested.connect(
            self._on_shared_view_selection_requested
        )

        # Update active display and view lists when display or view lists change
        for signal_name in ("item_added", "item_removed", "item_renamed"):
            # fmt: off
            getattr(self.view_edit.display_model, signal_name).connect(
                lambda *args, **kwargs:
                self.active_display_view_edit.active_display_edit.reset()
            )
            getattr(self.view_edit.model, signal_name).connect(
                lambda *args, **kwargs:
                self.active_display_view_edit.active_view_edit.reset()
            )
            # fmt: on

    @property
    def splitter(self) -> QtWidgets.QSplitter:
        return self.tabs.currentWidget().splitter

    def set_splitter_sizes(self, from_sizes: list[int]) -> None:
        """
        Update splitter to match the provided sizes.

        :param from_sizes: Sizes to match, with emphasis on matching
            the first index.
        """
        to_widget = self.tabs.currentWidget()
        to_widget.splitter.setSizes(
            adapt_splitter_sizes(from_sizes, to_widget.splitter.sizes())
        )

    @QtCore.Slot(int)
    def _on_current_changed(self, index: int) -> None:
        """Match tab splitter sizes on tab change."""
        from_widget = self.tabs.widget(self._prev_index)
        to_widget = self.tabs.widget(index)

        to_widget.splitter.setSizes(
            adapt_splitter_sizes(
                from_widget.splitter.sizes(), to_widget.splitter.sizes()
            )
        )

        self._prev_index = index

    @QtCore.Slot(str)
    def _on_shared_view_selection_requested(self, view: str) -> None:
        """
        Switch to the shared view tab and try to make the named view
        current.
        """
        self.tabs.setCurrentWidget(self.shared_view_edit)
        self.shared_view_edit.set_current_view(view)
