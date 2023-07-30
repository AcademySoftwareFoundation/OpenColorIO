# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from PySide6 import QtCore, QtWidgets

from ..config_cache import ConfigCache
from ..utils import get_glyph_icon
from ..widgets import (
    CallbackComboBox,
    ExpandingStackedWidget,
    FormLayout,
    LineEdit,
    StringMapTableWidget,
)
from .file_rule_model import FileRuleType, FileRuleModel
from .config_item_edit import BaseConfigItemParamEdit, BaseConfigItemEdit


class FileRuleParamEdit(BaseConfigItemParamEdit):
    """Widget for editing the parameters for one file rule."""

    __model_type__ = FileRuleModel
    __has_transforms__ = False

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        self._current_file_rule_type = FileRuleType.RULE_DEFAULT

        # Build stack widget layers
        self._param_stack = ExpandingStackedWidget()

        self.name_edits = {}
        self.color_space_combos = {}
        self.pattern_edits = {}
        self.regex_edits = {}
        self.extension_edits = {}
        self.custom_keys_tables = {}

        for file_rule_type in FileRuleType.__members__.values():
            params_layout = FormLayout()
            params_layout.setContentsMargins(0, 0, 0, 0)

            name_edit = LineEdit()
            self.name_edits[file_rule_type] = name_edit
            params_layout.addRow(self.model.NAME.label, name_edit)

            if file_rule_type != FileRuleType.RULE_OCIO_V1:
                color_space_combo = CallbackComboBox(ConfigCache.get_color_space_names)
                self.color_space_combos[file_rule_type] = color_space_combo
                params_layout.addRow(self.model.COLOR_SPACE.label, color_space_combo)

            if file_rule_type == FileRuleType.RULE_BASIC:
                pattern_edit = LineEdit()
                self.pattern_edits[file_rule_type] = pattern_edit
                params_layout.addRow(self.model.PATTERN.label, pattern_edit)

                extension_edit = LineEdit()
                self.extension_edits[file_rule_type] = extension_edit
                params_layout.addRow(self.model.EXTENSION.label, extension_edit)

            if file_rule_type == FileRuleType.RULE_REGEX:
                regex_edit = LineEdit()
                self.regex_edits[file_rule_type] = regex_edit
                params_layout.addRow(self.model.REGEX.label, regex_edit)

            if file_rule_type in (FileRuleType.RULE_BASIC, FileRuleType.RULE_REGEX):
                custom_keys_table = StringMapTableWidget(
                    ("Key Name", "Key Value"),
                    item_icon=get_glyph_icon("ph.key"),
                    default_key_prefix="key_",
                    default_value="value",
                )
                self.custom_keys_tables[file_rule_type] = custom_keys_table
                params_layout.addRow(self.model.CUSTOM_KEYS.label, custom_keys_table)

            params = QtWidgets.QFrame()
            params.setLayout(params_layout)
            self._param_stack.addWidget(params)

        self._param_layout.removeRow(0)
        self._param_layout.addRow(self._param_stack)

    def update_available_params(
        self, mapper: QtWidgets.QDataWidgetMapper, file_rule_type: FileRuleType
    ) -> None:
        """
        Enable the interface needed to edit this rule's type.
        """
        self._current_file_rule_type = file_rule_type

        self._param_stack.setCurrentIndex(
            [
                FileRuleType.RULE_BASIC,
                FileRuleType.RULE_REGEX,
                FileRuleType.RULE_OCIO_V1,
                FileRuleType.RULE_DEFAULT,
            ].index(file_rule_type)
        )

        if file_rule_type in self.name_edits:
            mapper.addMapping(self.name_edits[file_rule_type], self.model.NAME.column)
            self.name_edits[file_rule_type].setEnabled(
                file_rule_type in (FileRuleType.RULE_BASIC, FileRuleType.RULE_REGEX)
            )

        if file_rule_type in self.color_space_combos:
            mapper.addMapping(
                self.color_space_combos[file_rule_type], self.model.COLOR_SPACE.column
            )
            self.color_space_combos[file_rule_type].setEnabled(
                file_rule_type != FileRuleType.RULE_OCIO_V1
            )

        if file_rule_type in self.pattern_edits:
            mapper.addMapping(
                self.pattern_edits[file_rule_type], self.model.PATTERN.column
            )
            self.pattern_edits[file_rule_type].setEnabled(
                file_rule_type == FileRuleType.RULE_BASIC
            )

        if file_rule_type in self.regex_edits:
            mapper.addMapping(self.regex_edits[file_rule_type], self.model.REGEX.column)
            self.regex_edits[file_rule_type].setEnabled(
                file_rule_type == FileRuleType.RULE_REGEX
            )

        if file_rule_type in self.extension_edits:
            mapper.addMapping(
                self.extension_edits[file_rule_type], self.model.EXTENSION.column
            )
            self.extension_edits[file_rule_type].setEnabled(
                file_rule_type == FileRuleType.RULE_BASIC
            )

        if file_rule_type in self.custom_keys_tables:
            mapper.addMapping(
                self.custom_keys_tables[file_rule_type], self.model.CUSTOM_KEYS.column
            )
            self.custom_keys_tables[file_rule_type].setEnabled(
                file_rule_type in (FileRuleType.RULE_BASIC, FileRuleType.RULE_REGEX)
            )


class FileRuleEdit(BaseConfigItemEdit):
    """
    Widget for editing all file rules in the current config.
    """

    __param_edit_type__ = FileRuleParamEdit

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        model = self.model

        # Clear default mapped widgets. Widgets will be remapped per file rule type.
        self._mapper.clearMapping()

        # Table widgets need manual data submission back to model
        for custom_keys_table in self.param_edit.custom_keys_tables.values():
            custom_keys_table.items_changed.connect(self._mapper.submit)

        # Initialize
        if model.rowCount():
            self.list.set_current_row(0)

    @QtCore.Slot(int)
    def _on_current_row_changed(self, row: int) -> None:
        if row != -1:
            # Update parameter widget states, since file rule type may differ from
            # the previous rule.
            file_rule_type = self.model.data(
                self.model.index(row, self.model.FILE_RULE_TYPE.column),
                QtCore.Qt.EditRole,
            )
            self.param_edit.update_available_params(self._mapper, file_rule_type)

        super()._on_current_row_changed(row)
