# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from functools import partial
from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtWidgets

from ..config_cache import ConfigCache
from ..widgets import CallbackComboBox, TextEdit
from .look_model import LookModel
from .config_item_edit import BaseConfigItemParamEdit, BaseConfigItemEdit


class LookParamEdit(BaseConfigItemParamEdit):
    """
    Widget for editing the parameters and transforms for one look.
    """

    __model_type__ = LookModel
    __has_transforms__ = True
    __from_ref_column_desc__ = LookModel.TRANSFORM
    __to_ref_column_desc__ = LookModel.INVERSE_TRANSFORM

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Widgets
        self.process_space_combo = CallbackComboBox(
            lambda: ConfigCache.get_color_space_names(ocio.SEARCH_REFERENCE_SPACE_SCENE)
        )
        self.description_edit = TextEdit()

        # Layout
        self._param_layout.addRow(
            self.model.PROCESS_SPACE.label, self.process_space_combo
        )
        self._param_layout.addRow(self.model.DESCRIPTION.label, self.description_edit)


class LookEdit(BaseConfigItemEdit):
    """
    Widget for editing all looks in the current config.
    """

    __param_edit_type__ = LookParamEdit

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        model = self.model

        # Map widgets to model columns
        self._mapper.addMapping(
            self.param_edit.process_space_combo, model.PROCESS_SPACE.column
        )
        self._mapper.addMapping(
            self.param_edit.description_edit, model.DESCRIPTION.column
        )

        # Trigger immediate update from widgets that update the model upon losing focus
        self.param_edit.process_space_combo.currentIndexChanged.connect(
            partial(self.param_edit.submit_mapper_deferred, self._mapper)
        )

        # Initialize
        if model.rowCount():
            self.list.set_current_row(0)
