# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from PySide6 import QtGui, QtWidgets

from ..signal_router import SignalRouter
from ..widgets import ItemModelTableWidget
from .delegates import RoleDelegate
from .role_model import RoleModel


class RoleEdit(QtWidgets.QWidget):
    """
    Widget for editing all color space roles in the current config.
    """

    @classmethod
    def item_type_icon(cls) -> QtGui.QIcon:
        """
        :return: Item type icon
        """
        return RoleModel.item_type_icon()

    @classmethod
    def item_type_label(cls, plural: bool = False) -> str:
        """
        :param plural: Whether label should be plural
        :return: Friendly type name
        """
        return RoleModel.item_type_label(plural=plural)

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        self.model = RoleModel()

        # Connect signal router to model change
        signal_router = SignalRouter.get_instance()
        self.model.dataChanged.connect(
            lambda *a, **kw: signal_router.emit_roles_changed()
        )
        self.model.item_renamed.connect(
            lambda *a, **kw: signal_router.emit_roles_changed()
        )
        self.model.item_added.connect(
            lambda *a, **kw: signal_router.emit_roles_changed()
        )
        self.model.item_moved.connect(
            lambda *a, **kw: signal_router.emit_roles_changed()
        )
        self.model.item_removed.connect(
            lambda *a, **kw: signal_router.emit_roles_changed()
        )

        # Widgets
        self.table = ItemModelTableWidget(self.model)
        self.table.view.setItemDelegate(RoleDelegate(self.model))

        # Layout
        tab_layout = QtWidgets.QVBoxLayout()
        tab_layout.addWidget(self.table)
        tab_frame = QtWidgets.QFrame()
        tab_frame.setLayout(tab_layout)

        self.tabs = QtWidgets.QTabWidget()
        self.tabs.addTab(
            tab_frame,
            RoleModel.item_type_icon(),
            RoleModel.item_type_label(plural=True),
        )

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.tabs)
        self.setLayout(layout)
