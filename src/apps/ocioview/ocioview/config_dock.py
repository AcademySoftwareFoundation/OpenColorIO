# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtWidgets

from .items import (
    ColorSpaceEdit,
    ConfigPropertiesEdit,
    DisplayViewEdit,
    LookEdit,
    NamedTransformEdit,
    RoleEdit,
    RuleEdit,
    ViewTransformEdit,
)
from .log_handlers import message_queue
from .utils import get_glyph_icon
from .widgets import TabbedDockWidget


class ConfigDock(TabbedDockWidget):
    """
    Dockable widget for editing the current config.
    """

    config_changed = QtCore.Signal()

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__("Config", get_glyph_icon("ph.file-text"), parent=parent)

        self._models = []

        self.setAllowedAreas(
            QtCore.Qt.LeftDockWidgetArea | QtCore.Qt.RightDockWidgetArea
        )
        self.tabs.setTabPosition(QtWidgets.QTabWidget.West)

        # Widgets
        self.properties_edit = ConfigPropertiesEdit()
        self._connect_config_item_model(self.properties_edit.model)

        self.role_edit = RoleEdit()
        self._connect_config_item_model(self.role_edit.model)

        self.rule_edit = RuleEdit()
        self._connect_config_item_model(self.rule_edit.file_rule_edit.model)
        self._connect_config_item_model(self.rule_edit.viewing_rule_edit.model)

        self.display_view_edit = DisplayViewEdit()
        self._connect_config_item_model(self.display_view_edit.view_edit.display_model)
        self._connect_config_item_model(self.display_view_edit.view_edit.model)
        self._connect_config_item_model(self.display_view_edit.shared_view_edit.model)
        self._connect_config_item_model(
            self.display_view_edit.active_display_view_edit.active_display_edit.model
        )
        self._connect_config_item_model(
            self.display_view_edit.active_display_view_edit.active_view_edit.model
        )

        self.look_edit = LookEdit()
        self._connect_config_item_model(self.look_edit.model)

        self.view_transform_edit = ViewTransformEdit()
        self._connect_config_item_model(self.view_transform_edit.model)

        self.color_space_edit = ColorSpaceEdit()
        self._connect_config_item_model(self.color_space_edit.model)

        self.named_transform_edit = NamedTransformEdit()
        self._connect_config_item_model(self.named_transform_edit.model)

        # Layout
        self.add_tab(
            self.properties_edit,
            self.properties_edit.item_type_label(),
            self.properties_edit.item_type_icon(),
        )
        self.add_tab(
            self.role_edit,
            f"Color Space {self.role_edit.item_type_label(plural=True)}",
            self.role_edit.item_type_icon(),
        )
        self.add_tab(
            self.rule_edit,
            self.rule_edit.item_type_label(plural=True),
            self.rule_edit.item_type_icon(),
        )
        self.add_tab(
            self.display_view_edit,
            self.display_view_edit.item_type_label(plural=True),
            self.display_view_edit.item_type_icon(),
        )
        self.add_tab(
            self.look_edit,
            self.look_edit.item_type_label(plural=True),
            self.look_edit.item_type_icon(),
        )
        self.add_tab(
            self.view_transform_edit,
            self.view_transform_edit.item_type_label(plural=True),
            self.view_transform_edit.item_type_icon(),
        )
        self.add_tab(
            self.color_space_edit,
            self.color_space_edit.item_type_label(plural=True),
            self.color_space_edit.item_type_icon(),
        )
        self.add_tab(
            self.named_transform_edit,
            self.named_transform_edit.item_type_label(plural=True),
            self.named_transform_edit.item_type_icon(),
        )

        # Initialize
        self.update_config_views()

    def reset(self) -> None:
        """Reset data for all config item models."""
        for model in self._models:
            model.reset()

        self.update_config_views()

    def update_config_views(self) -> None:
        """
        Push the current OCIO config into the message queue to give
        any listening config code view(s) an update.

        .. note::
            Views can also connect to the config_changed signal, but
            since the message queue is needed to HTML format the config
            YAML data, this short circuits that trip with a direct
            config update for relevant views.
        """
        message_queue.put_nowait(ocio.GetCurrentConfig())

    def _connect_config_item_model(self, model: QtCore.QAbstractItemModel) -> None:
        """
        Collect model and route all config changes to the
        'config_changed' signal.
        """
        self._models.append(model)

        model.dataChanged.connect(self._on_config_changed)
        model.item_added.connect(self._on_config_changed)
        model.item_moved.connect(self._on_config_changed)
        model.item_removed.connect(self._on_config_changed)
        model.warning_raised.connect(self._on_warning_raised)

    def _on_config_changed(self, *args, **kwargs) -> None:
        """
        Broadcast to the wider application that the config has changed.
        """
        self.config_changed.emit()
        self.update_config_views()

    def _on_warning_raised(self, message: str) -> None:
        """Raise item model warnings in a message box."""
        QtWidgets.QMessageBox.warning(self, "Warning", message)
