# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from pathlib import Path
from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtWidgets

from ..constants import RGB
from ..widgets import (
    ComboBox,
    LineEdit,
    PathEdit,
    FloatEditArray,
    TextEdit,
    StringListWidget,
    StringMapTableWidget,
)
from ..utils import get_glyph_icon
from .config_properties_model import ConfigPropertiesModel
from .config_item_edit import BaseConfigItemParamEdit, BaseConfigItemEdit


class ConfigPropertiesParamEdit(BaseConfigItemParamEdit):
    """
    Widget for editing root properties for the current config.
    """

    __model_type__ = ConfigPropertiesModel
    __has_transforms__ = False
    __has_tabs__ = True

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Widgets
        self.version_edit = ComboBox()
        self.version_edit.addItems(self.model.supported_versions())
        self.description_edit = TextEdit()
        self.env_vars_table = StringMapTableWidget(
            ("Name", "Default Value"),
            item_icon=get_glyph_icon("mdi6.variable"),
            default_key_prefix="ENV_VAR_",
            default_value="value",
        )
        self.search_path_list = StringListWidget(
            item_icon=get_glyph_icon("ph.file-search"), get_item=self._get_search_path
        )
        self.working_dir_edit = PathEdit(QtWidgets.QFileDialog.Directory)
        self.family_separator_edit = LineEdit()
        self.default_luma_coefs_edit = FloatEditArray(RGB)

        # Layout
        self._param_layout.addRow(self.model.VERSION.label, self.version_edit)
        self._param_layout.addRow(self.model.DESCRIPTION.label, self.description_edit)
        self._param_layout.addRow(
            self.model.ENVIRONMENT_VARS.label, self.env_vars_table
        )
        self._param_layout.addRow(self.model.SEARCH_PATH.label, self.search_path_list)
        self._param_layout.addRow(self.model.WORKING_DIR.label, self.working_dir_edit)
        self._param_layout.addRow(
            self.model.FAMILY_SEPARATOR.label, self.family_separator_edit
        )
        self._param_layout.addRow(
            self.model.DEFAULT_LUMA_COEFS.label, self.default_luma_coefs_edit
        )

    def _get_search_path(self) -> Optional[str]:
        """
        Browse filesystem for new search path, making the returned
        directory path relative to the working directory if it is a
        child directory.

        :return: Search path string to add, or None to bail
        """
        config = ocio.GetCurrentConfig()
        working_dir_str = config.getWorkingDir()

        search_path_str = QtWidgets.QFileDialog.getExistingDirectory(
            self, "Choose Search Path", dir=working_dir_str
        )
        if search_path_str:
            working_dir = Path(working_dir_str)
            search_path = Path(search_path_str)
            if search_path.is_relative_to(working_dir):
                return search_path.relative_to(working_dir).as_posix()
            else:
                return search_path.as_posix()

        return None


class ConfigPropertiesEdit(BaseConfigItemEdit):
    """
    Widget for editing root properties in the current config.
    """

    __param_edit_type__ = ConfigPropertiesParamEdit
    __has_list__ = False

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        model = self.model

        # Map widgets to model columns
        self._mapper.addMapping(self.param_edit.version_edit, model.VERSION.column)
        self._mapper.addMapping(
            self.param_edit.description_edit, model.DESCRIPTION.column
        )
        self._mapper.addMapping(
            self.param_edit.env_vars_table, model.ENVIRONMENT_VARS.column
        )
        self._mapper.addMapping(
            self.param_edit.search_path_list, model.SEARCH_PATH.column
        )
        self._mapper.addMapping(
            self.param_edit.working_dir_edit, model.WORKING_DIR.column
        )
        self._mapper.addMapping(
            self.param_edit.family_separator_edit, model.FAMILY_SEPARATOR.column
        )
        self._mapper.addMapping(
            self.param_edit.default_luma_coefs_edit,
            model.DEFAULT_LUMA_COEFS.column,
        )

        # Table and list widgets need manual data submission back to model
        self.param_edit.env_vars_table.items_changed.connect(self._mapper.submit)
        self.param_edit.search_path_list.items_changed.connect(self._mapper.submit)

        # Reload sole item on reset
        model.modelReset.connect(lambda: self._mapper.setCurrentIndex(0))

        # Initialize
        self._mapper.setCurrentIndex(0)
