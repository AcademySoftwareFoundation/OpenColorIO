# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import logging
import shutil
from pathlib import Path
from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from .config_cache import ConfigCache
from .config_dock import ConfigDock
from .constants import ICON_PATH_OCIO
from .inspect_dock import InspectDock
from .message_router import MessageRouter
from .settings import settings
from .undo import undo_stack
from .viewer_dock import ViewerDock


logger = logging.getLogger(__name__)


class OCIOView(QtWidgets.QMainWindow):
    """
    ocioview application main window.
    """

    # NOTE: Change this number when a major change to this widget's structure is
    #       implemented. This prevents conflicts when restoring QMainWindow state from
    #       settings.
    SETTING_STATE_VERSION = 1

    SETTING_GEOMETRY = "geometry"
    SETTING_STATE = "state"
    SETTING_CONFIG_DIR = "config_dir"
    SETTING_RECENT_CONFIGS = "recent_configs"
    SETTING_RECENT_CONFIG_PATH = "path"

    def __init__(
        self,
        config_path: Optional[Path] = None,
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param config_path: Optional OCIO config path to load. Defaults
            to the builtin raw config.
        """
        super().__init__(parent=parent)

        self._config_path = None
        self._config_save_cache_id = None

        # Configure window
        self.setWindowIcon(QtGui.QIcon(str(ICON_PATH_OCIO)))

        # Recent file menus
        self.recent_configs_menu = QtWidgets.QMenu("Load Recent Config")
        self.recent_images_menu = QtWidgets.QMenu("Load Recent Image")

        # Dock widgets
        self.inspect_dock = InspectDock()
        self.config_dock = ConfigDock()

        # Central widget
        self.viewer_dock = ViewerDock(self.recent_images_menu)

        # Main menu
        self.file_menu = QtWidgets.QMenu("File")
        self.file_menu.addAction("New Config", self.new_config)
        self.file_menu.addAction("Load Config...", self.load_config)
        self.file_menu.addMenu(self.recent_configs_menu)
        self.file_menu.addAction(
            "Save config", self.save_config, QtGui.QKeySequence("Ctrl+S")
        )
        self.file_menu.addAction(
            "Save Config As...", self.save_config_as, QtGui.QKeySequence("Ctrl+Shift+S")
        )
        self.file_menu.addAction(
            "Save and Backup Config",
            self.save_and_backup_config,
            QtGui.QKeySequence("Ctrl+Alt+S"),
        )
        self.file_menu.addAction("Restore Config Backup...", self.restore_config_backup)
        self.file_menu.addSeparator()
        self.file_menu.addAction(
            "Load Image...", self.viewer_dock.load_image, QtGui.QKeySequence("Ctrl+I")
        )
        self.file_menu.addMenu(self.recent_images_menu)
        self.file_menu.addAction(
            "Load Image in New Tab...",
            lambda: self.viewer_dock.load_image(new_tab=True),
            QtGui.QKeySequence("Ctrl+Shift+I"),
        )
        self.file_menu.addSeparator()
        self.file_menu.addAction("Exit", self.close, QtGui.QKeySequence("Ctrl+X"))

        self.edit_menu = QtWidgets.QMenu("Edit")
        undo_action = undo_stack.createUndoAction(self.edit_menu)
        undo_action.setShortcut(QtGui.QKeySequence("Ctrl+Z"))
        self.edit_menu.addAction(undo_action)
        redo_action = undo_stack.createRedoAction(self.edit_menu)
        redo_action.setShortcut(QtGui.QKeySequence("Ctrl+Shift+Z"))
        self.edit_menu.addAction(redo_action)
        self.edit_menu.addSeparator()

        self.menu_bar = QtWidgets.QMenuBar()
        self.menu_bar.addMenu(self.file_menu)
        self.menu_bar.addMenu(self.edit_menu)
        self.setMenuBar(self.menu_bar)

        # Dock areas
        self.setDockOptions(
            QtWidgets.QMainWindow.ForceTabbedDocks
            | QtWidgets.QMainWindow.GroupedDragging
        )
        self.setTabPosition(QtCore.Qt.BottomDockWidgetArea, QtWidgets.QTabWidget.North)
        self.setTabPosition(QtCore.Qt.LeftDockWidgetArea, QtWidgets.QTabWidget.North)
        self.setTabPosition(QtCore.Qt.RightDockWidgetArea, QtWidgets.QTabWidget.North)

        for corner in (QtCore.Qt.TopLeftCorner, QtCore.Qt.BottomLeftCorner):
            self.setCorner(corner, QtCore.Qt.LeftDockWidgetArea)
        for corner in (QtCore.Qt.TopRightCorner, QtCore.Qt.BottomRightCorner):
            self.setCorner(corner, QtCore.Qt.RightDockWidgetArea)

        # Layout
        self.setCentralWidget(self.viewer_dock)
        self.addDockWidget(QtCore.Qt.BottomDockWidgetArea, self.inspect_dock)
        self.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.config_dock)

        # Connections
        self.config_dock.config_changed.connect(self.viewer_dock.update_current_viewer)
        self.config_dock.config_changed.connect(self._update_window_title)

        # Restore settings
        settings.beginGroup(self.__class__.__name__)
        if settings.contains(self.SETTING_GEOMETRY):
            self.restoreGeometry(settings.value(self.SETTING_GEOMETRY))
        if settings.contains(self.SETTING_STATE):
            # If the version is not recognized, the restore will be bypassed
            self.restoreState(
                settings.value(self.SETTING_STATE), version=self.SETTING_STATE_VERSION
            )
        settings.endGroup()

        # Initialize
        if config_path is not None:
            self.load_config(config_path)

        self._update_recent_configs_menu()
        self._update_window_title()

        # Start log processing
        MessageRouter.get_instance().start_routing()

    def reset(self) -> None:
        """
        Reset application, reloading the current OCIO config.
        """
        self.config_dock.reset()
        self.viewer_dock.reset()
        self.inspect_dock.reset()

    def closeEvent(self, event: QtGui.QCloseEvent) -> None:
        if self._can_close_config():
            # Save settings
            settings.beginGroup(self.__class__.__name__)
            settings.setValue(self.SETTING_GEOMETRY, self.saveGeometry())
            settings.setValue(
                self.SETTING_STATE, self.saveState(self.SETTING_STATE_VERSION)
            )
            settings.endGroup()

            event.accept()
            super().closeEvent(event)
        else:
            event.ignore()

    def new_config(self) -> None:
        """
        Create and load a new OCIO raw config.
        """
        if not self._can_close_config():
            return

        self._config_path = None
        self._config_save_cache_id = None

        config = ocio.Config.CreateRaw()
        ocio.SetCurrentConfig(config)

        self.reset()

    def load_config(self, config_path: Optional[Path] = None) -> None:
        """
        Load a user specified OCIO config.

        :param config_path: Config file path
        """
        if not self._can_close_config():
            return

        if config_path is None or not config_path.is_file():
            config_dir = self._get_config_dir(config_path)
            config_path_str, file_filter = QtWidgets.QFileDialog.getOpenFileName(
                self, "Load Config", dir=config_dir, filter="OCIO Config (*.ocio)"
            )
            if not config_path_str:
                return

            config_path = Path(config_path_str)
            settings.setValue(self.SETTING_CONFIG_DIR, config_path.parent.as_posix())

        self._config_path = config_path

        # Add path to recent config files
        self._add_recent_config_path(self._config_path)

        # Reset application with empty config to clean all components
        config = ocio.Config()
        ocio.SetCurrentConfig(config)
        self.reset()

        # Reset application again to update all components with the new config
        config = ocio.Config.CreateFromFile(self._config_path.as_posix())
        ocio.SetCurrentConfig(config)
        self.reset()

        self._update_cache_id()

    def save_config(self) -> bool:
        """
        Save the current OCIO config to the previously loaded config
        path. If no config has been loaded, 'save_config_as' will be
        called.

        :return: Whether config was saved
        """
        if self._config_path is None:
            return self.save_config_as()
        else:
            try:
                config_dir = self._config_path.parent
                config_dir.mkdir(parents=True, exist_ok=True)

                config = ocio.GetCurrentConfig()
                config.serialize(self._config_path.as_posix())

                self._update_cache_id()
                return True

            except Exception as e:
                QtWidgets.QMessageBox.critical(
                    self, "Error", f"Config save failed with error: {str(e)}"
                )
                logger.error(str(e), exc_info=e)

        return False

    def save_config_as(self, config_path: Optional[Path] = None) -> bool:
        """
        Save the current OCIO config to a user specified path.

        :param config_path: Config file path
        :return: Whether config was saved
        """
        try:
            if config_path is None or not config_path.is_file():
                config_dir = self._get_config_dir(config_path)
                config_path_str, file_filter = QtWidgets.QFileDialog.getSaveFileName(
                    self,
                    "Save Config",
                    dir=config_dir,
                    filter="OCIO Config (*.ocio)",
                )
                if not config_path_str:
                    return False

                config_path = Path(config_path_str)

            self._config_path = config_path

            # Add path to recent config files
            self._add_recent_config_path(self._config_path)

            config = ocio.GetCurrentConfig()
            config.serialize(self._config_path.as_posix())

            self._update_cache_id()
            return True

        except Exception as e:
            QtWidgets.QMessageBox.critical(
                self, "Error", f"Config save failed with error: {str(e)}"
            )
            logger.error(str(e), exc_info=e)

        return False

    def save_and_backup_config(self) -> bool:
        """
        Save the config and make an incremental copy in a 'backup'
        directory beside the config file.

        :return: Whether config was saved
        """
        if self.save_config():
            try:
                if self._config_path is not None and self._config_path.is_file():
                    next_version_path = self._get_next_version_path()
                    shutil.copy2(self._config_path, next_version_path)
                    return True

            except Exception as e:
                QtWidgets.QMessageBox.critical(
                    self, "Error", f"Config backup failed with error: {str(e)}"
                )
                logger.error(str(e), exc_info=e)

        return False

    def restore_config_backup(self) -> None:
        """
        Browse for a config version from the 'backup' directory, and
        restore it to memory after backing up the current config.
        Calling save after restoring will save the restored config to
        disk, making it the current config.
        """
        if not self._can_close_config():
            return

        backup_dir = self._get_backup_dir()
        if backup_dir is not None:
            version_path_str, file_filter = QtWidgets.QFileDialog.getOpenFileName(
                self,
                "Restore Config",
                dir=backup_dir.as_posix(),
                filter="OCIO Config (*.ocio)",
            )
            if not version_path_str:
                return

            version_path = Path(version_path_str)
            current_path = self._config_path

            # Backup current config to a new version and load the requested backup
            # config in memory.
            self.save_and_backup_config()
            self.load_config(version_path)

            # Keep the internal config path set to the non-backup config path. If the
            # user chooses to save, the loaded backup will become the current config
            # version.
            self._config_path = current_path

    def _get_next_version_path(self) -> Optional[Path]:
        """
        Get the path to next backup version of the config.

        :return: Config version path
        """
        backup_dir = self._get_backup_dir()
        if backup_dir is None:
            return None

        max_version = 0
        for other_version_path in backup_dir.glob(self._format_version_filename()):
            if other_version_path.is_file() and other_version_path.suffixes:
                other_version_str = other_version_path.suffixes[0].strip(".")
                if other_version_str.isdigit():
                    other_version = int(other_version_str)
                    if other_version > max_version:
                        max_version = other_version

        return backup_dir / self._format_version_filename(max_version + 1)

    def _format_version_filename(
        self, version_num: Optional[int] = None
    ) -> Optional[str]:
        """
        Format a config version filename, given a version number.

        :param version_num: Version number
        :return: Config version filename
        """
        if self._config_path is not None:
            return (
                f"{self._config_path.stem}."
                f"{'*' if not version_num else f'{version_num:04d}'}"
                f"{self._config_path.suffix}"
            )
        else:
            return None

    def _get_backup_dir(self) -> Optional[Path]:
        """
        :return: Config backup directory, which is created if it
            doesn't exist yet.
        """
        if self._config_path is not None and self._config_path.is_file():
            backup_dir = self._config_path.parent / "backup"
            backup_dir.mkdir(parents=True, exist_ok=True)
            return backup_dir
        else:
            return None

    def _get_config_dir(self, config_path: Optional[Path] = None) -> str:
        """
        Infer a config save/load directory from an existing config path
        or settings.
        """
        config_dir = ""
        if config_path is not None:
            config_dir = config_path.parent.as_posix()
        if not config_dir and self._config_path is not None:
            config_dir = self._config_path.parent.as_posix()
        if not config_dir and settings.contains(self.SETTING_CONFIG_DIR):
            config_dir = settings.value(self.SETTING_CONFIG_DIR)
        return config_dir

    def _get_recent_config_paths(self) -> list[Path]:
        """
        Get the 10 most recently loaded or saved config file paths that
        still exist.

        :return: List of OCIO config file paths
        """
        recent_configs = []

        num_configs = settings.beginReadArray(self.SETTING_RECENT_CONFIGS)
        for i in range(num_configs):
            settings.setArrayIndex(i)
            recent_config_path_str = settings.value(self.SETTING_RECENT_CONFIG_PATH)
            if recent_config_path_str:
                recent_config_path = Path(recent_config_path_str)
                if recent_config_path.is_file():
                    recent_configs.append(recent_config_path)
        settings.endArray()

        return recent_configs

    def _add_recent_config_path(self, config_path: Path) -> None:
        """
        Add the provided config file path to the top of the recent
        config files list.

        :param config_path: OCIO config file path
        """
        config_paths = self._get_recent_config_paths()
        if config_path in config_paths:
            config_paths.remove(config_path)
        config_paths.insert(0, config_path)

        if len(config_paths) > 10:
            config_paths = config_path[:10]

        settings.beginWriteArray(self.SETTING_RECENT_CONFIGS)
        for i, recent_config_path in enumerate(config_paths):
            settings.setArrayIndex(i)
            settings.setValue(
                self.SETTING_RECENT_CONFIG_PATH, recent_config_path.as_posix()
            )
        settings.endArray()

        # Update menu with latest list
        self._update_recent_configs_menu()

    def _update_recent_configs_menu(self) -> None:
        """Update recent configs menu actions."""
        self.recent_configs_menu.clear()
        for recent_config_path in self._get_recent_config_paths():
            self.recent_configs_menu.addAction(
                recent_config_path.name,
                lambda path=recent_config_path: self.load_config(path),
            )

    def _update_window_title(self) -> None:
        filename = (
            "untitiled" if self._config_path is None else self._config_path.name
        ) + ("*" if self._has_unsaved_changes() else "")

        self.setWindowTitle(f"ocioview {ocio.__version__} | {filename}")

    def _update_cache_id(self):
        """
        Update cache ID which represents config state at the last save,
        for determining whether unsaved changes exist.
        """
        config_cache_id, is_valid = ConfigCache.get_cache_id()
        if is_valid:
            self._config_save_cache_id = config_cache_id
            self._update_window_title()

    def _has_unsaved_changes(self) -> bool:
        """
        :return: Whether the current config has unsaved changes, when
        compared to the previously saved config state.
        """
        config_cache_id, is_valid = ConfigCache.get_cache_id()
        return not is_valid or config_cache_id != self._config_save_cache_id

    def _can_close_config(self) -> bool:
        """
        Ask user if changes should be saved.

        :return: True if changes were saved or discarded, in which case
            the config can be closed, or False if the operation was
            cancelled and the config should remain open for editing.
        """
        if self._has_unsaved_changes():
            button = QtWidgets.QMessageBox.warning(
                self,
                "Save Changes?",
                "The current config has been modified. Would you like to save your "
                "changes before closing? All unsaved changes will be lost if "
                "discarded.",
                QtWidgets.QMessageBox.Save
                | QtWidgets.QMessageBox.Discard
                | QtWidgets.QMessageBox.Cancel,
                QtWidgets.QMessageBox.Cancel,
            )
            if button == QtWidgets.QMessageBox.Save:
                # Save changes. Ok to close config if save is successful.
                return self.save_config()
            elif button == QtWidgets.QMessageBox.Discard:
                # Changes discarded. Ok to close config.
                return True
            else:
                # Changes not saved or discarded. Keep editing confing.
                return False

        # No unsaved changes
        return True
