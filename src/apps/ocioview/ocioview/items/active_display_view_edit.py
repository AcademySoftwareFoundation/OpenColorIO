# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from PySide6 import QtCore, QtGui, QtWidgets

from ..widgets import ItemModelListWidget
from ..utils import get_glyph_icon
from .active_display_view_model import ActiveDisplayModel, ActiveViewModel


class ActiveDisplayEdit(ItemModelListWidget):
    """Widget for editing active displays in the current config."""

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        self.model = ActiveDisplayModel()

        super().__init__(
            self.model,
            self.model.NAME.column,
            item_icon=self.model.item_type_icon(),
            items_constant=True,
            parent=parent,
        )

        self.model.item_selection_requested.connect(
            lambda index: self.set_current_row(index.row())
        )


class ActiveViewEdit(ItemModelListWidget):
    """Widget for editing active views in the current config."""

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        self.model = ActiveViewModel()

        super().__init__(
            self.model,
            self.model.NAME.column,
            item_icon=self.model.item_type_icon(),
            items_constant=True,
            parent=parent,
        )

        self.model.item_selection_requested.connect(
            lambda index: self.set_current_row(index.row())
        )


class ActiveDisplayViewEdit(QtWidgets.QWidget):
    """
    Widget for editing the active display and view lists for the
    current config.

    .. note::
        The active display and view edits control display and view
        visibility and order in an application's UI.
    """

    @classmethod
    def item_type_icon(cls) -> QtGui.QIcon:
        return get_glyph_icon("mdi6.sort-bool-ascending-variant")

    @classmethod
    def item_type_label(cls, plural: bool = False) -> str:
        return f"Active Display{'s' if plural else ''} and View{'s' if plural else ''}"

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Widgets
        self.active_display_edit = ActiveDisplayEdit()
        self.active_view_edit = ActiveViewEdit()

        # Layout
        self.splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)
        self.splitter.setOpaqueResize(False)
        self.splitter.addWidget(self.active_display_edit)
        self.splitter.addWidget(self.active_view_edit)

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.splitter)
        self.setLayout(layout)
