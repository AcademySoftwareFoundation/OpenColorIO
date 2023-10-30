# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from functools import partial
from typing import Optional

from PySide6 import QtWidgets
import PyOpenColorIO as ocio

from ..config_cache import ConfigCache
from ..utils import get_glyph_icon
from ..widgets import EnumComboBox, CallbackComboBox, StringListWidget, TextEdit
from .config_item_edit import BaseConfigItemParamEdit, BaseConfigItemEdit
from .view_transform_model import ViewTransformModel


class ViewTransformParamEdit(BaseConfigItemParamEdit):
    """
    Widget for editing the parameters and transforms for one view
    transform.
    """

    __model_type__ = ViewTransformModel
    __has_transforms__ = True
    __from_ref_column_desc__ = ViewTransformModel.FROM_REFERENCE
    __to_ref_column_desc__ = ViewTransformModel.TO_REFERENCE

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Widgets
        self.reference_space_type_combo = EnumComboBox(
            ocio.ReferenceSpaceType,
            icons={
                ocio.REFERENCE_SPACE_SCENE: get_glyph_icon("ph.sun"),
                ocio.REFERENCE_SPACE_DISPLAY: get_glyph_icon("ph.monitor"),
            },
        )
        self.family_edit = CallbackComboBox(ConfigCache.get_families, editable=True)
        self.description_edit = TextEdit()
        self.categories_list = StringListWidget(
            item_basename="category",
            item_icon=get_glyph_icon("ph.bookmarks-simple"),
            get_presets=self._get_available_categories,
        )

        # Layout
        self._param_layout.addRow(
            self.model.REFERENCE_SPACE_TYPE.label, self.reference_space_type_combo
        )
        self._param_layout.addRow(self.model.FAMILY.label, self.family_edit)
        self._param_layout.addRow(self.model.DESCRIPTION.label, self.description_edit)
        self._param_layout.addRow(self.model.CATEGORIES.label, self.categories_list)

    def _get_available_categories(self) -> list[str]:
        """
        :return: All unused categories which can be added as presets
            to this item.
        """
        current_categories = self.categories_list.items()
        return [c for c in ConfigCache.get_categories() if c not in current_categories]


class ViewTransformEdit(BaseConfigItemEdit):
    """
    Widget for editing all view transforms in the current config.
    """

    __param_edit_type__ = ViewTransformParamEdit

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        model = self.model

        # Map widgets to model columns
        self._mapper.addMapping(
            self.param_edit.reference_space_type_combo,
            model.REFERENCE_SPACE_TYPE.column,
        )
        self._mapper.addMapping(self.param_edit.family_edit, model.FAMILY.column)
        self._mapper.addMapping(
            self.param_edit.description_edit, model.DESCRIPTION.column
        )
        self._mapper.addMapping(
            self.param_edit.categories_list, model.CATEGORIES.column
        )

        # list widgets need manual data submission back to model
        self.param_edit.categories_list.items_changed.connect(self._mapper.submit)

        # Trigger immediate update from widgets that update the model upon losing focus
        self.param_edit.reference_space_type_combo.currentIndexChanged.connect(
            partial(self.param_edit.submit_mapper_deferred, self._mapper)
        )

        # Initialize
        if model.rowCount():
            self.list.set_current_row(0)
