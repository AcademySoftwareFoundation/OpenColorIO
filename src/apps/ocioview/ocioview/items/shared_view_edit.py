# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from functools import partial
from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtWidgets

from ..config_cache import ConfigCache
from ..widgets import CallbackComboBox, LineEdit
from .config_item_edit import BaseConfigItemParamEdit, BaseConfigItemEdit
from .shared_view_model import SharedViewModel


class SharedViewParamEdit(BaseConfigItemParamEdit):
    """Widget for editing the parameters for a shared view."""

    __model_type__ = SharedViewModel
    __has_transforms__ = False

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Widgets
        self.color_space_combo = CallbackComboBox(
            lambda: [ocio.OCIO_VIEW_USE_DISPLAY_NAME]
            + ConfigCache.get_color_space_names(ocio.SEARCH_REFERENCE_SPACE_DISPLAY)
        )
        self.view_transform_combo = CallbackComboBox(
            ConfigCache.get_view_transform_names
        )
        self.looks_edit = LineEdit()
        self.rule_combo = CallbackComboBox(ConfigCache.get_viewing_rule_names)
        self.description_edit = LineEdit()

        # Layout
        self._param_layout.addRow(self.model.COLOR_SPACE.label, self.color_space_combo)
        self._param_layout.addRow(
            self.model.VIEW_TRANSFORM.label, self.view_transform_combo
        )
        self._param_layout.addRow(self.model.LOOKS.label, self.looks_edit)
        self._param_layout.addRow(self.model.RULE.label, self.rule_combo)
        self._param_layout.addRow(self.model.DESCRIPTION.label, self.description_edit)


class SharedViewEdit(BaseConfigItemEdit):
    """
    Widget for editing all displays and views in the current config.
    """

    __param_edit_type__ = SharedViewParamEdit

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        model = self.model

        # Map widgets to model columns
        self._mapper.addMapping(
            self.param_edit.color_space_combo, model.COLOR_SPACE.column
        )
        self._mapper.addMapping(
            self.param_edit.view_transform_combo, model.VIEW_TRANSFORM.column
        )
        self._mapper.addMapping(self.param_edit.looks_edit, model.LOOKS.column)
        self._mapper.addMapping(self.param_edit.rule_combo, model.RULE.column)
        self._mapper.addMapping(
            self.param_edit.description_edit, model.DESCRIPTION.column
        )

        # Trigger immediate update from widgets that update the model upon losing focus
        self.param_edit.color_space_combo.currentIndexChanged.connect(
            partial(self.param_edit.submit_mapper_deferred, self._mapper)
        )
        self.param_edit.view_transform_combo.currentIndexChanged.connect(
            partial(self.param_edit.submit_mapper_deferred, self._mapper)
        )

        # Initialize
        if model.rowCount():
            self.list.set_current_row(0)

    def set_current_view(self, view: str) -> None:
        """Set the current shared view."""
        self.list.set_current_item(view)
