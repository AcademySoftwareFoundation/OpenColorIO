# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import enum
from dataclasses import dataclass, field
from typing import Any, Optional, Union

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui

from ..config_cache import ConfigCache
from ..undo import ConfigSnapshotUndoCommand
from ..utils import get_glyph_icon, next_name
from .config_item_model import ColumnDesc, BaseConfigItemModel


class FileRuleType(str, enum.Enum):
    """Enum of file rule types."""

    RULE_BASIC = "Basic Rule"
    RULE_REGEX = "Regex Rule"
    RULE_OCIO_V1 = "OCIO v1 Style Rule"
    RULE_DEFAULT = "Default Rule"


@dataclass
class FileRule:
    """Individual file rule storage."""

    type: FileRuleType
    name: str
    color_space: str = ""
    pattern: str = ""
    regex: str = ""
    extension: str = ""
    custom_keys: dict[str, str] = field(default_factory=dict)

    def args(self) -> Union[tuple[str, str, str, str], tuple[str, str, str]]:
        """
        Return tuple of *args for
        ``FileRules.insertRule(index, *args)``, which will correspond
        to a basic or regex rule, the two variations of this overloaded
        function.
        """
        if self.type == FileRuleType.RULE_REGEX:
            return self.name, self.color_space, self.regex
        else:  # FileRuleType.RULE_BASIC:
            return self.name, self.color_space, self.pattern, self.extension


class FileRuleModel(BaseConfigItemModel):
    """
    Item model for editing file rules in the current config.
    """

    FILE_RULE_TYPE = ColumnDesc(0, "File Rule Type", str)
    NAME = ColumnDesc(1, "Name", str)
    COLOR_SPACE = ColumnDesc(2, "Color Space", str)
    PATTERN = ColumnDesc(3, "Pattern", str)
    REGEX = ColumnDesc(4, "Regex", str)
    EXTENSION = ColumnDesc(5, "Extension", str)
    CUSTOM_KEYS = ColumnDesc(6, "Custom Keys", list)

    COLUMNS = sorted(
        [FILE_RULE_TYPE, NAME, COLOR_SPACE, PATTERN, REGEX, EXTENSION, CUSTOM_KEYS],
        key=lambda s: s.column,
    )

    __item_type__ = FileRule
    __icon_glyph__ = "mdi6.file-check-outline"

    @classmethod
    def get_rule_type_icon(cls, rule_type: FileRuleType) -> QtGui.QIcon:
        glyph_names = {
            FileRuleType.RULE_BASIC: "ph.asterisk",
            FileRuleType.RULE_REGEX: "msc.regex",
            FileRuleType.RULE_OCIO_V1: "mdi6.contain",
            FileRuleType.RULE_DEFAULT: "ph.arrow-line-down",
        }
        return get_glyph_icon(glyph_names[rule_type])

    @classmethod
    def has_presets(cls) -> bool:
        return True

    @classmethod
    def requires_presets(cls) -> bool:
        return True

    @classmethod
    def get_presets(cls) -> Optional[Union[list[str], dict[str, QtGui.QIcon]]]:
        return {
            FileRuleType.RULE_BASIC.value: cls.get_rule_type_icon(
                FileRuleType.RULE_BASIC
            ),
            FileRuleType.RULE_REGEX.value: cls.get_rule_type_icon(
                FileRuleType.RULE_REGEX
            ),
            FileRuleType.RULE_OCIO_V1.value: cls.get_rule_type_icon(
                FileRuleType.RULE_OCIO_V1
            ),
        }

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        self._rule_type_icons = {
            FileRuleType.RULE_BASIC: self.get_rule_type_icon(FileRuleType.RULE_BASIC),
            FileRuleType.RULE_REGEX: self.get_rule_type_icon(FileRuleType.RULE_REGEX),
            FileRuleType.RULE_OCIO_V1: self.get_rule_type_icon(
                FileRuleType.RULE_OCIO_V1
            ),
            FileRuleType.RULE_DEFAULT: self.get_rule_type_icon(
                FileRuleType.RULE_DEFAULT
            ),
        }

        ConfigCache.register_reset_callback(self._reset_cache)

    def add_preset(self, preset_name: str) -> int:
        file_rules = self._get_editable_file_rules()
        all_names = self.get_item_names()

        if preset_name == FileRuleType.RULE_BASIC.value:
            item = FileRule(
                FileRuleType.RULE_BASIC,
                next_name("BasicRule_", all_names),
                ConfigCache.get_default_color_space_name(),
                pattern="*",
                extension="*",
            )
        elif preset_name == FileRuleType.RULE_REGEX.value:
            item = FileRule(
                FileRuleType.RULE_REGEX,
                next_name("RegexRule_", all_names),
                ConfigCache.get_default_color_space_name(),
                regex=".*",
            )
        else:  # FileRuleType.RULE_OCIO_V1.value
            # Only one instance of this rule is allowed
            if ocio.FILE_PATH_SEARCH_RULE_NAME not in all_names:
                item = FileRule(
                    FileRuleType.RULE_OCIO_V1,
                    ocio.FILE_PATH_SEARCH_RULE_NAME,
                    ConfigCache.get_default_color_space_name(),
                )
            else:
                return file_rules.getIndexForRule(ocio.FILE_PATH_SEARCH_RULE_NAME)

        # Make new rule top priority
        row = 0

        with ConfigSnapshotUndoCommand(
            f"Add {self.item_type_label()}", model=self, item_name=item.name
        ):
            self.beginInsertRows(self.NULL_INDEX, row, row)
            self._insert_rule(row, file_rules, item)

            ocio.GetCurrentConfig().setFileRules(file_rules)

            self.endInsertRows()
            self.item_added.emit(item.name)

        return row

    def move_item_up(self, item_name: str) -> bool:
        """
        Increase priority (index - 1) for the named rule.
        """
        if item_name != ocio.DEFAULT_RULE_NAME:
            file_rules = self._get_editable_file_rules()
            src_rule_index = file_rules.getIndexForRule(item_name)
            dst_rule_index = max(0, src_rule_index - 1)

            if src_rule_index != dst_rule_index:
                if not self.beginMoveRows(
                    QtCore.QModelIndex(),
                    src_rule_index,
                    src_rule_index,
                    QtCore.QModelIndex(),
                    dst_rule_index,
                ):
                    return False

                with ConfigSnapshotUndoCommand(
                    f"Move {self.item_type_label()}", model=self, item_name=item_name
                ):
                    file_rules.increaseRulePriority(src_rule_index)
                    ocio.GetCurrentConfig().setFileRules(file_rules)

                    self.endMoveRows()
                    self.item_moved.emit()

                return True

        return False

    def move_item_down(self, item_name: str) -> bool:
        """
        Decrease priority (index + 1) for the named rule.
        """
        if item_name != ocio.DEFAULT_RULE_NAME:
            file_rules = self._get_editable_file_rules()
            rule_count = file_rules.getNumEntries()
            src_rule_index = file_rules.getIndexForRule(item_name)
            dst_rule_index = min(rule_count - 2, src_rule_index + 1)

            if src_rule_index != dst_rule_index:
                if not self.beginMoveRows(
                    QtCore.QModelIndex(),
                    src_rule_index,
                    src_rule_index,
                    QtCore.QModelIndex(),
                    dst_rule_index + 1,
                ):
                    return False

                with ConfigSnapshotUndoCommand(
                    f"Move {self.item_type_label()}", model=self, item_name=item_name
                ):
                    file_rules.decreaseRulePriority(src_rule_index)
                    ocio.GetCurrentConfig().setFileRules(file_rules)

                    self.endMoveRows()
                    self.item_moved.emit()

                return True

        return False

    def get_item_names(self) -> list[str]:
        config = ocio.GetCurrentConfig()
        file_rules = config.getFileRules()

        return [file_rules.getName(i) for i in range(file_rules.getNumEntries())]

    def _get_icon(
        self, item: FileRule, column_desc: ColumnDesc
    ) -> Optional[QtGui.QIcon]:
        if column_desc == self.NAME:
            return self._rule_type_icons[item.type]
        else:
            return None

    def _reset_cache(self) -> None:
        self._items = []

    def _get_items(self, preserve: bool = False) -> list[FileRule]:
        if ConfigCache.validate() and self._items:
            return self._items

        config = ocio.GetCurrentConfig()
        file_rules = config.getFileRules()
        self._items = []

        for i in range(file_rules.getNumEntries()):
            name = file_rules.getName(i)
            color_space = file_rules.getColorSpace(i)
            pattern = file_rules.getPattern(i)
            regex = file_rules.getRegex(i)
            extension = file_rules.getExtension(i)

            if name == ocio.DEFAULT_RULE_NAME:
                file_rule_type = FileRuleType.RULE_DEFAULT
            elif name == ocio.FILE_PATH_SEARCH_RULE_NAME:
                file_rule_type = FileRuleType.RULE_OCIO_V1
            elif regex:
                file_rule_type = FileRuleType.RULE_REGEX
            else:
                file_rule_type = FileRuleType.RULE_BASIC

            custom_keys = {}
            for j in range(file_rules.getNumCustomKeys(i)):
                key_name = file_rules.getCustomKeyName(i, j)
                key_value = file_rules.getCustomKeyValue(i, j)
                custom_keys[key_name] = key_value

            self._items.append(
                FileRule(
                    file_rule_type,
                    name,
                    color_space,
                    pattern,
                    regex,
                    extension,
                    custom_keys,
                )
            )

        return self._items

    def _clear_items(self) -> None:
        ocio.GetCurrentConfig().setFileRules(ocio.FileRules())

    @staticmethod
    def _insert_rule(index: int, file_rules: ocio.FileRules, item: FileRule) -> None:
        """
        Insert rule into an ``ocio.FileRules`` object from a FileRule
        instance.
        """
        if item.name == ocio.FILE_PATH_SEARCH_RULE_NAME:
            file_rules.insertPathSearchRule(index)
        elif item.name == ocio.DEFAULT_RULE_NAME:
            file_rules.setDefaultRuleColorSpace(item.color_space)
        else:
            file_rules.insertRule(index, *item.args())
            for key_name, key_value in item.custom_keys.items():
                file_rules.setCustomKey(index, key_name, key_value)

    def _remove_named_rule(self, file_rules: ocio.ViewingRules, item: FileRule) -> None:
        """Remove existing rule with name matching the provided rule."""
        # Default rule can't be removed
        if item.name != ocio.DEFAULT_RULE_NAME:
            for i in range(file_rules.getNumEntries()):
                if file_rules.getName(i) == item.name:
                    file_rules.removeRule(i)
                    break

    def _get_editable_file_rules(self) -> ocio.FileRules:
        """
        Copy existing config rules into new editable ``ocio.FileRules``
        instance.
        """
        file_rules = ocio.FileRules()
        for i, item in enumerate(self._get_items()):
            self._insert_rule(i, file_rules, item)
        return file_rules

    def _add_item(self, item: FileRule) -> None:
        # Only presets can be added
        pass

    def _remove_item(self, item: FileRule) -> None:
        file_rules = self._get_editable_file_rules()
        self._remove_named_rule(file_rules, item)
        ocio.GetCurrentConfig().setFileRules(file_rules)

    def _new_item(self, name: str) -> None:
        # Only presets can be added
        pass

    def _get_value(self, item: FileRule, column_desc: ColumnDesc) -> Any:
        # Get parameters
        if column_desc == self.FILE_RULE_TYPE:
            return item.type
        if column_desc == self.NAME:
            return item.name
        elif column_desc == self.COLOR_SPACE:
            return item.color_space
        elif column_desc == self.PATTERN:
            return item.pattern
        elif column_desc == self.REGEX:
            return item.regex
        elif column_desc == self.EXTENSION:
            return item.extension
        elif column_desc == self.CUSTOM_KEYS:
            return list(item.custom_keys.items())

        # Invalid column
        return None

    def _set_value(
        self,
        item: FileRule,
        column_desc: ColumnDesc,
        value: Any,
        index: QtCore.QModelIndex,
    ) -> None:
        file_rules = self._get_editable_file_rules()
        current_index = file_rules.getIndexForRule(item.name)
        prev_item_name = item.name

        # Update parameters
        if column_desc == self.NAME:
            if item.type in (FileRuleType.RULE_BASIC, FileRuleType.RULE_REGEX):
                # Name must be unique
                if value not in self.get_item_names():
                    # Remove rule with previous name before adding new one
                    self._remove_named_rule(file_rules, item)
                    item.name = value
                    self._insert_rule(current_index, file_rules, item)

        elif column_desc == self.COLOR_SPACE:
            if item.type != FileRuleType.RULE_OCIO_V1:
                file_rules.setColorSpace(current_index, value)
        elif column_desc == self.PATTERN:
            if item.type == FileRuleType.RULE_BASIC:
                file_rules.setPattern(current_index, value)
        elif column_desc == self.REGEX:
            if item.type == FileRuleType.RULE_REGEX:
                file_rules.setRegex(current_index, value)
        elif column_desc == self.EXTENSION:
            if item.type == FileRuleType.RULE_BASIC:
                file_rules.setExtension(current_index, value)

        elif column_desc == self.CUSTOM_KEYS:
            if item.type in (FileRuleType.RULE_BASIC, FileRuleType.RULE_REGEX):
                item.custom_keys.clear()
                for key_name, key_value in value:
                    item.custom_keys[key_name] = key_value

                # Need to re-add rule to replace custom keys
                self._remove_named_rule(file_rules, item)
                self._insert_rule(current_index, file_rules, item)

        ocio.GetCurrentConfig().setFileRules(file_rules)

        if item.name != prev_item_name:
            self.item_renamed.emit(item.name, prev_item_name)
