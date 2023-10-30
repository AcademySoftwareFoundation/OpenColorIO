# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from PySide6 import QtCore, QtWidgets

from ..config_cache import ConfigCache
from ..utils import get_glyph_icon
from ..widgets import (
    FormLayout,
    LineEdit,
    StringListWidget,
    StringMapTableWidget,
    ExpandingStackedWidget,
)
from .config_item_edit import BaseConfigItemParamEdit, BaseConfigItemEdit
from .delegates import ColorSpaceDelegate
from .viewing_rule_model import ViewingRuleType, ViewingRuleModel


class ViewingRuleParamEdit(BaseConfigItemParamEdit):
    """Widget for editing the parameters for one viewing rule."""

    __model_type__ = ViewingRuleModel
    __has_transforms__ = False

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Widgets
        self.name_edit_a = LineEdit()
        self.color_space_list = StringListWidget(
            item_basename="color space",
            item_flags=QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable,
            item_icon=ViewingRuleModel.get_rule_type_icon(
                ViewingRuleType.RULE_COLOR_SPACE
            ),
            allow_empty=False,
            get_presets=ConfigCache.get_color_space_names,
            presets_only=True,
        )
        self.color_space_list.view.setItemDelegate(ColorSpaceDelegate())
        self.custom_keys_table_a = StringMapTableWidget(
            ("Key Name", "Key Value"),
            item_icon=get_glyph_icon("ph.key"),
            default_key_prefix="key_",
            default_value="value",
        )

        self.name_edit_b = LineEdit()
        self.encoding_list = StringListWidget(
            item_basename="encoding",
            item_flags=QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable,
            item_icon=ViewingRuleModel.get_rule_type_icon(
                ViewingRuleType.RULE_ENCODING
            ),
            allow_empty=False,
            get_presets=ConfigCache.get_encodings,
            presets_only=True,
        )
        self.custom_keys_table_b = StringMapTableWidget(
            ("Key Name", "Key Value"),
            item_icon=get_glyph_icon("ph.key"),
            default_key_prefix="key_",
            default_value="value",
        )

        # Layout
        no_params = QtWidgets.QFrame()

        params_layout_a = FormLayout()
        params_layout_a.setContentsMargins(0, 0, 0, 0)
        params_layout_a.addRow(self.model.NAME.label, self.name_edit_a)
        params_layout_a.addRow(self.model.COLOR_SPACES.label, self.color_space_list)
        params_layout_a.addRow(self.model.CUSTOM_KEYS.label, self.custom_keys_table_a)
        params_a = QtWidgets.QFrame()
        params_a.setLayout(params_layout_a)

        params_layout_b = FormLayout()
        params_layout_b.setContentsMargins(0, 0, 0, 0)
        params_layout_b.addRow(self.model.NAME.label, self.name_edit_b)
        params_layout_b.addRow(self.model.ENCODINGS.label, self.encoding_list)
        params_layout_b.addRow(self.model.CUSTOM_KEYS.label, self.custom_keys_table_b)
        params_b = QtWidgets.QFrame()
        params_b.setLayout(params_layout_b)

        self._param_stack = ExpandingStackedWidget()
        self._param_stack.addWidget(no_params)
        self._param_stack.addWidget(params_a)
        self._param_stack.addWidget(params_b)

        self._param_layout.removeRow(0)
        self._param_layout.addRow(self._param_stack)

        self.model.item_removed.connect(self._on_item_removed)

    def reset(self) -> None:
        super().reset()
        self._param_stack.setCurrentIndex(0)

    def update_available_params(
        self, mapper: QtWidgets.QDataWidgetMapper, viewing_rule_type: ViewingRuleType
    ) -> None:
        """
        Map and show the interface needed to edit this rule's type.
        """
        if viewing_rule_type == ViewingRuleType.RULE_COLOR_SPACE:
            mapper.addMapping(self.name_edit_a, self.model.NAME.column)
            mapper.addMapping(self.custom_keys_table_a, self.model.CUSTOM_KEYS.column)
            self._param_stack.setCurrentIndex(1)

        else:  # ViewingRuleType.RULE_ENCODING
            mapper.addMapping(self.name_edit_b, self.model.NAME.column)
            mapper.addMapping(self.custom_keys_table_b, self.model.CUSTOM_KEYS.column)
            self._param_stack.setCurrentIndex(2)

    def _on_item_removed(self) -> None:
        """Hide rule widgets when no rule is present."""
        if not self.model.rowCount():
            self._param_stack.setCurrentIndex(0)


class ViewingRuleEdit(BaseConfigItemEdit):
    """
    Widget for editing all viewing rules in the current config.
    """

    __param_edit_type__ = ViewingRuleParamEdit

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        model = self.model

        # Map widgets to model columns
        self._mapper.addMapping(
            self.param_edit.color_space_list, model.COLOR_SPACES.column
        )
        self._mapper.addMapping(self.param_edit.encoding_list, model.ENCODINGS.column)

        # list and table widgets need manual data submission back to model
        self.param_edit.color_space_list.items_changed.connect(self._mapper.submit)
        self.param_edit.encoding_list.items_changed.connect(self._mapper.submit)
        self.param_edit.custom_keys_table_a.items_changed.connect(self._mapper.submit)
        self.param_edit.custom_keys_table_b.items_changed.connect(self._mapper.submit)

        # Initialize
        if model.rowCount():
            self.list.set_current_row(0)

    @QtCore.Slot(int)
    def _on_current_row_changed(self, row: int) -> None:
        if row != -1:
            # Update parameter widget states, since viewing rule type may differ from
            # the previous rule.
            viewing_rule_type = self.model.data(
                self.model.index(row, self.model.VIEWING_RULE_TYPE.column),
                QtCore.Qt.EditRole,
            )
            self.param_edit.update_available_params(self._mapper, viewing_rule_type)

        super()._on_current_row_changed(row)
