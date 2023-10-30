# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from functools import partial
from typing import Optional

from PySide6 import QtCore, QtWidgets
import PyOpenColorIO as ocio

from ..config_cache import ConfigCache
from ..utils import get_glyph_icon
from ..widgets import (
    CheckBox,
    EnumComboBox,
    CallbackComboBox,
    FloatEditArray,
    StringListWidget,
    TextEdit,
)
from .color_space_model import ColorSpaceModel
from .config_item_edit import BaseConfigItemParamEdit, BaseConfigItemEdit


class ColorSpaceParamEdit(BaseConfigItemParamEdit):
    """
    Widget for editing the parameters and transforms for one color
    space.
    """

    __model_type__ = ColorSpaceModel
    __has_transforms__ = True
    __from_ref_column_desc__ = ColorSpaceModel.FROM_REFERENCE
    __to_ref_column_desc__ = ColorSpaceModel.TO_REFERENCE

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
        self.aliases_list = StringListWidget(
            item_basename="alias", item_icon=get_glyph_icon("ph.bookmark-simple")
        )
        self.family_edit = CallbackComboBox(ConfigCache.get_families, editable=True)
        self.encoding_edit = CallbackComboBox(ConfigCache.get_encodings, editable=True)
        self.equality_group_edit = CallbackComboBox(
            ConfigCache.get_equality_groups, editable=True
        )
        self.description_edit = TextEdit()
        self.bit_depth_combo = EnumComboBox(ocio.BitDepth)
        self.is_data_check = CheckBox()
        self.allocation_combo = EnumComboBox(ocio.Allocation)
        self.allocation_combo.currentIndexChanged.connect(self._on_allocation_changed)
        self.allocation_vars_edit = FloatEditArray(
            ("min", "max", "offset"), (0.0, 1.0, 0.0)
        )
        self.categories_list = StringListWidget(
            item_basename="category",
            item_icon=get_glyph_icon("ph.bookmarks-simple"),
            get_presets=self._get_available_categories,
        )

        # Layout
        self._param_layout.addRow(
            self.model.REFERENCE_SPACE_TYPE.label, self.reference_space_type_combo
        )
        self._param_layout.addRow(self.model.ALIASES.label, self.aliases_list)
        self._param_layout.addRow(self.model.FAMILY.label, self.family_edit)
        self._param_layout.addRow(self.model.ENCODING.label, self.encoding_edit)
        self._param_layout.addRow(
            self.model.EQUALITY_GROUP.label, self.equality_group_edit
        )
        self._param_layout.addRow(self.model.DESCRIPTION.label, self.description_edit)
        self._param_layout.addRow(self.model.BIT_DEPTH.label, self.bit_depth_combo)
        self._param_layout.addRow(self.model.IS_DATA.label, self.is_data_check)
        self._param_layout.addRow(self.model.ALLOCATION.label, self.allocation_combo)
        self._param_layout.addRow(
            self.model.ALLOCATION_VARS.label, self.allocation_vars_edit
        )
        self._param_layout.addRow(self.model.CATEGORIES.label, self.categories_list)

    def update_available_allocation_vars(self) -> None:
        """
        Enable the interface needed to edit this color space's allocation
        vars for the current allocation type.
        """
        allocation = self.allocation_combo.member()

        for i in range(2):
            self.allocation_vars_edit.value_edits[i].setEnabled(
                allocation != ocio.ALLOCATION_UNKNOWN
            )
        self.allocation_vars_edit.value_edits[2].setEnabled(
            allocation == ocio.ALLOCATION_LG2
        )

    @QtCore.Slot(int)
    def _on_allocation_changed(self, index: int) -> None:
        self.update_available_allocation_vars()

    def _get_available_categories(self) -> list[str]:
        """
        :return: All unused categories which can be added as presets
            to this item.
        """
        current_categories = self.categories_list.items()
        return [c for c in ConfigCache.get_categories() if c not in current_categories]


class ColorSpaceEdit(BaseConfigItemEdit):
    """
    Widget for editing all color spaces in the current config.
    """

    __param_edit_type__ = ColorSpaceParamEdit

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        model = self.model

        # Map widgets to model columns
        self._mapper.addMapping(
            self.param_edit.reference_space_type_combo,
            model.REFERENCE_SPACE_TYPE.column,
        )
        self._mapper.addMapping(self.param_edit.aliases_list, model.ALIASES.column)
        self._mapper.addMapping(self.param_edit.family_edit, model.FAMILY.column)
        self._mapper.addMapping(self.param_edit.encoding_edit, model.ENCODING.column)
        self._mapper.addMapping(
            self.param_edit.equality_group_edit, model.EQUALITY_GROUP.column
        )
        self._mapper.addMapping(
            self.param_edit.description_edit, model.DESCRIPTION.column
        )
        self._mapper.addMapping(self.param_edit.bit_depth_combo, model.BIT_DEPTH.column)
        self._mapper.addMapping(self.param_edit.is_data_check, model.IS_DATA.column)
        self._mapper.addMapping(
            self.param_edit.allocation_combo, model.ALLOCATION.column
        )
        self._mapper.addMapping(
            self.param_edit.allocation_vars_edit, model.ALLOCATION_VARS.column
        )
        self._mapper.addMapping(
            self.param_edit.categories_list, model.CATEGORIES.column
        )

        # list widgets need manual data submission back to model
        self.param_edit.aliases_list.items_changed.connect(self._mapper.submit)
        self.param_edit.categories_list.items_changed.connect(self._mapper.submit)

        # Trigger immediate update from widgets that update the model upon losing focus
        self.param_edit.reference_space_type_combo.currentIndexChanged.connect(
            partial(self.param_edit.submit_mapper_deferred, self._mapper)
        )
        self.param_edit.is_data_check.stateChanged.connect(
            partial(self.param_edit.submit_mapper_deferred, self._mapper)
        )

        # Initialize
        if model.rowCount():
            self.list.set_current_row(0)

    @QtCore.Slot(int)
    def _on_current_row_changed(self, row: int) -> None:
        super()._on_current_row_changed(row)

        if row != -1:
            # Update allocation var widget states, since allocation may differ from
            # the previous color space.
            self.param_edit.update_available_allocation_vars()
