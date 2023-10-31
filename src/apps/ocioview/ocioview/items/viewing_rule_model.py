# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import copy
import enum
from dataclasses import dataclass, field
from typing import Any, Optional, Union

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui

from ..config_cache import ConfigCache
from ..undo import ConfigSnapshotUndoCommand
from ..utils import get_glyph_icon, next_name
from .config_item_model import ColumnDesc, BaseConfigItemModel


class ViewingRuleType(str, enum.Enum):
    """Enum of viewing rule types."""

    RULE_COLOR_SPACE = "Color Space Rule"
    RULE_ENCODING = "Encoding Rule"


@dataclass
class ViewingRule:
    """Individual viewing rule storage."""

    type: ViewingRuleType
    name: str
    color_spaces: list[str] = field(default_factory=list)
    encodings: list[str] = field(default_factory=list)
    custom_keys: dict[str, str] = field(default_factory=dict)


class ViewingRuleModel(BaseConfigItemModel):
    """
    Item model for editing viewing rules in the current config.
    """

    VIEWING_RULE_TYPE = ColumnDesc(0, "Viewing Rule Type", str)
    NAME = ColumnDesc(1, "Name", str)
    COLOR_SPACES = ColumnDesc(2, "Color Spaces", list)
    ENCODINGS = ColumnDesc(3, "Encodings", list)
    CUSTOM_KEYS = ColumnDesc(4, "Custom Keys", list)

    COLUMNS = sorted(
        [VIEWING_RULE_TYPE, NAME, COLOR_SPACES, ENCODINGS, CUSTOM_KEYS],
        key=lambda s: s.column,
    )

    __item_type__ = ViewingRule
    __icon_glyph__ = "mdi6.eye-check-outline"

    @classmethod
    def get_rule_type_icon(cls, rule_type: ViewingRuleType) -> QtGui.QIcon:
        glyph_names = {
            ViewingRuleType.RULE_COLOR_SPACE: "ph.swap",
            ViewingRuleType.RULE_ENCODING: "mdi6.sine-wave",
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
            ViewingRuleType.RULE_COLOR_SPACE.value: cls.get_rule_type_icon(
                ViewingRuleType.RULE_COLOR_SPACE
            ),
            ViewingRuleType.RULE_ENCODING.value: cls.get_rule_type_icon(
                ViewingRuleType.RULE_ENCODING
            ),
        }

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        self._rule_types = {}

        self._rule_type_icons = {
            ViewingRuleType.RULE_COLOR_SPACE: self.get_rule_type_icon(
                ViewingRuleType.RULE_COLOR_SPACE
            ),
            ViewingRuleType.RULE_ENCODING: self.get_rule_type_icon(
                ViewingRuleType.RULE_ENCODING
            ),
        }

        ConfigCache.register_reset_callback(self._reset_cache)

    def add_preset(self, preset_name: str) -> int:
        viewing_rules = self._get_editable_viewing_rules()
        all_names = self.get_item_names()
        item = None

        if preset_name == ViewingRuleType.RULE_COLOR_SPACE.value:
            color_space = ConfigCache.get_default_color_space_name()
            if not color_space:
                color_spaces = ConfigCache.get_color_spaces()
                if color_spaces:
                    color_space = color_spaces[0]
            if color_space:
                item = ViewingRule(
                    ViewingRuleType.RULE_COLOR_SPACE,
                    next_name("ColorSpaceRule_", all_names),
                    color_spaces=[color_space],
                )
            else:
                self.warning_raised.emit(
                    f"Could not create "
                    f"{ViewingRuleType.RULE_COLOR_SPACE.value.lower()} because no "
                    f"color spaces are defined."
                )

        else:  # ViewingRuleType.RULE_ENCODING.value:
            encodings = ConfigCache.get_encodings()
            item = ViewingRule(
                ViewingRuleType.RULE_ENCODING,
                next_name("EncodingRule_", all_names),
                encodings=[encodings[0]],
            )

        # Put new rule at top
        row = -1
        if item is not None:
            row = 0

            with ConfigSnapshotUndoCommand(
                f"Add {self.item_type_label()}", model=self, item_name=item.name
            ):
                self.beginInsertRows(self.NULL_INDEX, row, row)
                self._insert_rule(row, viewing_rules, item)

                ocio.GetCurrentConfig().setViewingRules(viewing_rules)

                self.endInsertRows()
                self.item_added.emit(item.name)

        return row

    def get_item_names(self) -> list[str]:
        config = ocio.GetCurrentConfig()
        viewing_rules = config.getViewingRules()

        return [viewing_rules.getName(i) for i in range(viewing_rules.getNumEntries())]

    def _get_icon(
        self, item: ViewingRule, column_desc: ColumnDesc
    ) -> Optional[QtGui.QIcon]:
        if column_desc == self.NAME:
            return self._rule_type_icons[item.type]
        else:
            return None

    def _reset_cache(self) -> None:
        self._items = []

    def _get_items(self, preserve: bool = False) -> list[ViewingRule]:
        if ConfigCache.validate() and self._items:
            return self._items

        config = ocio.GetCurrentConfig()
        viewing_rules = config.getViewingRules()
        self._items = []

        for i in range(viewing_rules.getNumEntries()):
            name = viewing_rules.getName(i)
            color_spaces = list(viewing_rules.getColorSpaces(i))
            encodings = list(viewing_rules.getEncodings(i))

            if color_spaces:
                viewing_rule_type = ViewingRuleType.RULE_COLOR_SPACE
            elif encodings:
                viewing_rule_type = ViewingRuleType.RULE_ENCODING
            else:
                # Ambiguous rule type; drop it.
                continue

            custom_keys = {}
            for j in range(viewing_rules.getNumCustomKeys(i)):
                key_name = viewing_rules.getCustomKeyName(i, j)
                key_value = viewing_rules.getCustomKeyValue(i, j)
                custom_keys[key_name] = key_value

            self._items.append(
                ViewingRule(
                    viewing_rule_type, name, color_spaces, encodings, custom_keys
                )
            )

        return self._items

    def _clear_items(self) -> None:
        ocio.GetCurrentConfig().setViewingRules(ocio.ViewingRules())

    def _insert_rule(
        self, index: int, viewing_rules: ocio.ViewingRules, item: ViewingRule
    ) -> None:
        """
        Insert rule into an ``ocio.ViewingRules`` object from a
        ViewingRule instance.
        """
        viewing_rules.insertRule(index, item.name)

        if item.type == ViewingRuleType.RULE_COLOR_SPACE:
            for color_space in item.color_spaces:
                viewing_rules.addColorSpace(index, color_space)
        else:  # ViewingRuleType.RULE_ENCODING
            for encoding in item.encodings:
                viewing_rules.addEncoding(index, encoding)

        for key_name, key_value in item.custom_keys.items():
            viewing_rules.setCustomKey(index, key_name, key_value)

        self._rule_types[item.name] = item.type

    def _remove_named_rule(
        self, viewing_rules: ocio.ViewingRules, item: ViewingRule
    ) -> None:
        """Remove existing rule with name matching the provided rule."""
        for i in range(viewing_rules.getNumEntries()):
            if viewing_rules.getName(i) == item.name:
                viewing_rules.removeRule(i)
                break

        self._rule_types.pop(item.name, None)

    def _get_editable_viewing_rules(self) -> ocio.ViewingRules:
        """
        Copy existing config rules into new editable
        ``ocio.ViewingRules`` instance.
        """
        viewing_rules = ocio.ViewingRules()
        for i, item in enumerate(self._get_items()):
            self._insert_rule(i, viewing_rules, item)
        return viewing_rules

    def _add_item(self, item: ViewingRule) -> None:
        viewing_rules = self._get_editable_viewing_rules()
        self._insert_rule(viewing_rules.getNumEntries(), viewing_rules, item)
        ocio.GetCurrentConfig().setViewingRules(viewing_rules)

    def _new_item(self, name: str) -> None:
        # Only presets can be added
        pass

    def _remove_item(self, item: ViewingRule) -> None:
        viewing_rules = self._get_editable_viewing_rules()
        self._remove_named_rule(viewing_rules, item)
        ocio.GetCurrentConfig().setViewingRules(viewing_rules)

    def _get_value(self, item: ViewingRule, column_desc: ColumnDesc) -> Any:
        # Get parameters
        if column_desc == self.VIEWING_RULE_TYPE:
            return item.type
        elif column_desc == self.NAME:
            return item.name
        elif column_desc == self.COLOR_SPACES:
            return item.color_spaces
        elif column_desc == self.ENCODINGS:
            return item.encodings
        elif column_desc == self.CUSTOM_KEYS:
            return list(item.custom_keys.items())

        # Invalid column
        return None

    def _set_value(
        self,
        item: ViewingRule,
        column_desc: ColumnDesc,
        value: Any,
        index: QtCore.QModelIndex,
    ) -> None:
        viewing_rules = self._get_editable_viewing_rules()
        current_index = viewing_rules.getIndexForRule(item.name)
        prev_item = copy.deepcopy(item)

        # Update parameters
        if column_desc == self.NAME:
            # Name must be unique
            if value not in self.get_item_names():
                item.name = value

        elif column_desc == self.COLOR_SPACES:
            if item.type == ViewingRuleType.RULE_COLOR_SPACE:
                item.color_spaces = value
        elif column_desc == self.ENCODINGS:
            if item.type == ViewingRuleType.RULE_ENCODING:
                item.encodings = value

        elif column_desc == self.CUSTOM_KEYS:
            item.custom_keys.clear()
            for key_name, key_value in value:
                item.custom_keys[key_name] = key_value

        # Replace rule to update value
        self._remove_named_rule(viewing_rules, prev_item)
        self._insert_rule(current_index, viewing_rules, item)

        ocio.GetCurrentConfig().setViewingRules(viewing_rules)

        if item.name != prev_item.name:
            self.item_renamed.emit(item.name, prev_item.name)
