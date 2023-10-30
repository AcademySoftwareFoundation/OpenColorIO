# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import logging
from pathlib import Path
from typing import Any, Optional

from PySide6 import QtCore
import PyOpenColorIO as ocio

from .config_item_model import ColumnDesc, BaseConfigItemModel


logger = logging.getLogger(__name__)


class ConfigPropertiesModel(BaseConfigItemModel):
    """
    Item model for editing root properties in the current config.
    """

    NAME = ColumnDesc(0, "Name", str)
    VERSION = ColumnDesc(1, "Version", str)
    DESCRIPTION = ColumnDesc(2, "Description", str)
    ENVIRONMENT_VARS = ColumnDesc(3, "Environment Vars", list)
    SEARCH_PATH = ColumnDesc(4, "Search Path", list)
    WORKING_DIR = ColumnDesc(5, "Working Dir", Path)
    FAMILY_SEPARATOR = ColumnDesc(6, "Family Separator", str)
    DEFAULT_LUMA_COEFS = ColumnDesc(7, "Default Luma Coefs", list)

    # fmt: off
    COLUMNS = sorted([
        NAME, VERSION, DESCRIPTION, ENVIRONMENT_VARS, SEARCH_PATH, WORKING_DIR,
        FAMILY_SEPARATOR, DEFAULT_LUMA_COEFS,
    ], key=lambda s: s.column)
    # fmt: on

    __item_type__ = ocio.Config
    __icon_glyph__ = "ph.sliders"

    @classmethod
    def item_type_label(cls, plural: bool = False) -> str:
        return "Properties"

    def supported_versions(self) -> list[str]:
        """
        Infer all supported config versions from a test config.
        """
        versions = []
        test_config = ocio.Config()

        major_version = 1
        while True:
            try:
                # Setting the major version will set the minor version to the most
                # recent value.
                test_config.setMajorVersion(major_version)
                max_minor_version = test_config.getMinorVersion()

                # Iterate minor versions at and below the inferred maximum to test
                # which are valid.
                for minor_version in range(max_minor_version + 1):
                    try:
                        # Test the validity of this major/minor pair, adding it to the
                        # output if no exception is raised.
                        test_config.setVersion(major_version, minor_version)
                        versions.append(f"{major_version}.{minor_version}")
                    except ocio.Exception:
                        continue

            except ocio.Exception:
                break
            major_version += 1

        return versions

    def get_item_names(self) -> list[str]:
        return []

    def _get_item_and_column(
        self, index: QtCore.QModelIndex
    ) -> tuple[Optional[__item_type__], Optional[ColumnDesc]]:
        return ocio.GetCurrentConfig(), self.COLUMNS[index.column()]

    def _get_value(self, item: ocio.Config, column_desc: ColumnDesc) -> Any:
        # Get parameters
        if column_desc == self.NAME:
            return item.getName()
        elif column_desc == self.VERSION:
            return f"{item.getMajorVersion()}.{item.getMinorVersion()}"
        elif column_desc == self.DESCRIPTION:
            return item.getDescription()
        elif column_desc == self.ENVIRONMENT_VARS:
            return [
                (name, item.getEnvironmentVarDefault(name))
                for name in item.getEnvironmentVarNames()
            ]
        elif column_desc == self.SEARCH_PATH:
            return item.getSearchPaths()
        elif column_desc == self.WORKING_DIR:
            return Path(item.getWorkingDir())
        elif column_desc == self.FAMILY_SEPARATOR:
            return item.getFamilySeparator()
        elif column_desc == self.DEFAULT_LUMA_COEFS:
            return item.getDefaultLumaCoefs()

        # Invalid column
        return None

    def _set_value(
        self,
        item: ocio.Config,
        column_desc: ColumnDesc,
        value: Any,
        index: QtCore.QModelIndex,
    ) -> None:
        # Update parameters
        if column_desc == self.NAME:
            item.setName(value)
        elif column_desc == self.VERSION:
            major, minor = tuple(map(int, value.split(".")))
            try:
                item.setVersion(major, minor)
            except ocio.Exception as e:
                logger.warning(str(e))
        elif column_desc == self.DESCRIPTION:
            item.setDescription(value)
        elif column_desc == self.ENVIRONMENT_VARS:
            item.clearEnvironmentVars()
            for name, default in value:
                item.addEnvironmentVar(name, default)
        elif column_desc == self.SEARCH_PATH:
            item.clearSearchPaths()
            for path in value:
                item.addSearchPath(Path(path).as_posix())
        elif column_desc == self.WORKING_DIR:
            item.setWorkingDir(value.as_posix())
        elif column_desc == self.FAMILY_SEPARATOR:
            item.setFamilySeparator(value)
        elif column_desc == self.DEFAULT_LUMA_COEFS:
            item.setDefaultLumaCoefs(value)

    # There's one singleton config, so short circuit methods that aren't needed
    def rowCount(self, *args, **kwargs) -> int:
        return 1

    def insertRows(self, *args, **kwargs) -> bool:
        return False

    def removeRows(self, *args, **kwargs) -> bool:
        return False

    def _get_items(self, *args, **kwargs) -> list:
        return []

    def _clear_items(self, *args, **kwargs) -> None:
        pass

    def _add_item(self, *args, **kwargs) -> None:
        pass

    def _remove_item(self, *args, **kwargs) -> None:
        pass

    def _new_item(self, *args, **kwargs) -> None:
        pass
