# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from PySide6 import QtCore, QtGui, QtWidgets

from ..utils import get_glyph_icon
from .file_rule_edit import FileRuleEdit
from .utils import adapt_splitter_sizes
from .viewing_rule_edit import ViewingRuleEdit


class RuleEdit(QtWidgets.QWidget):
    """
    Widget for editing all file and viewing rules in the current
    config.
    """

    @classmethod
    def item_type_icon(cls) -> QtGui.QIcon:
        """
        :return: Item type icon
        """
        return get_glyph_icon("mdi6.list-status")

    @classmethod
    def item_type_label(cls, plural: bool = False) -> str:
        """
        :param plural: Whether label should be plural
        :return: Friendly type name
        """
        label = "Rule"
        if plural:
            label += "s"
        return label

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Widgets
        self.file_rule_edit = FileRuleEdit()
        self.viewing_rule_edit = ViewingRuleEdit()

        # Layout
        self.tabs = QtWidgets.QTabWidget()
        self.tabs.addTab(
            self.file_rule_edit,
            self.file_rule_edit.item_type_icon(),
            self.file_rule_edit.item_type_label(plural=True),
        )
        self.tabs.addTab(
            self.viewing_rule_edit,
            self.viewing_rule_edit.item_type_icon(),
            self.viewing_rule_edit.item_type_label(plural=True),
        )
        self.tabs.currentChanged.connect(self._on_current_changed)

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.tabs)
        self.setLayout(layout)

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
        from_widget = self.tabs.widget(1 - index)
        to_widget = self.tabs.widget(index)

        to_widget.splitter.setSizes(
            adapt_splitter_sizes(
                from_widget.splitter.sizes(), to_widget.splitter.sizes()
            )
        )
