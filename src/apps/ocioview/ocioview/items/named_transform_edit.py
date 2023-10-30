# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from PySide6 import QtWidgets

from ..config_cache import ConfigCache
from ..utils import get_glyph_icon
from ..widgets import CallbackComboBox, StringListWidget, TextEdit
from .config_item_edit import BaseConfigItemParamEdit, BaseConfigItemEdit
from .named_transform_model import NamedTransformModel


class NamedTransformParamEdit(BaseConfigItemParamEdit):
    """
    Widget for editing the parameters and transforms for one named
    transform.
    """

    __model_type__ = NamedTransformModel
    __has_transforms__ = True
    __from_ref_column_desc__ = NamedTransformModel.FORWARD_TRANSFORM
    __to_ref_column_desc__ = NamedTransformModel.INVERSE_TRANSFORM

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Widgets
        self.aliases_list = StringListWidget(
            item_basename="alias", item_icon=get_glyph_icon("ph.bookmark-simple")
        )
        self.family_edit = CallbackComboBox(ConfigCache.get_families, editable=True)
        self.encoding_edit = CallbackComboBox(ConfigCache.get_encodings, editable=True)
        self.description_edit = TextEdit()
        self.categories_list = StringListWidget(
            item_basename="category",
            item_icon=get_glyph_icon("ph.bookmarks-simple"),
            get_presets=self._get_available_categories,
        )

        # Layout
        self._param_layout.addRow(self.model.ALIASES.label, self.aliases_list)
        self._param_layout.addRow(self.model.FAMILY.label, self.family_edit)
        self._param_layout.addRow(self.model.ENCODING.label, self.encoding_edit)
        self._param_layout.addRow(self.model.DESCRIPTION.label, self.description_edit)
        self._param_layout.addRow(self.model.CATEGORIES.label, self.categories_list)

    def _get_available_categories(self) -> list[str]:
        """
        :return: All unused categories which can be added as presets
            to this item.
        """
        current_categories = self.categories_list.items()
        return [c for c in ConfigCache.get_categories() if c not in current_categories]


class NamedTransformEdit(BaseConfigItemEdit):
    """
    Widget for editing all named transforms in the current config.
    """

    __param_edit_type__ = NamedTransformParamEdit

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        model = self.model

        # Map widgets to model columns
        self._mapper.addMapping(self.param_edit.aliases_list, model.ALIASES.column)
        self._mapper.addMapping(self.param_edit.family_edit, model.FAMILY.column)
        self._mapper.addMapping(self.param_edit.encoding_edit, model.ENCODING.column)
        self._mapper.addMapping(
            self.param_edit.description_edit, model.DESCRIPTION.column
        )
        self._mapper.addMapping(
            self.param_edit.categories_list, model.CATEGORIES.column
        )

        # list widgets need manual data submission back to model
        self.param_edit.aliases_list.items_changed.connect(self._mapper.submit)
        self.param_edit.categories_list.items_changed.connect(self._mapper.submit)

        # Initialize
        if model.rowCount():
            self.list.set_current_row(0)
