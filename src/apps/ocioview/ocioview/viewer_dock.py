# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from collections import defaultdict
from pathlib import Path
from typing import Optional

from PySide6 import QtCore, QtWidgets

from .settings import settings
from .transform_manager import TransformManager
from .utils import get_glyph_icon
from .viewer import ImageViewer
from .widgets.structure import TabbedDockWidget


class ViewerDock(TabbedDockWidget):
    """
    Dockable widget for viewing color transforms on images.
    """

    SETTING_IMAGE_DIR = "image_dir"
    SETTING_RECENT_IMAGES = "recent_images"
    SETTING_RECENT_IMAGE_PATH = "path"

    def __init__(
        self,
        recent_images_menu: QtWidgets.QMenu,
        parent: Optional[QtCore.QObject] = None,
    ):
        super().__init__(
            "Viewer", get_glyph_icon("mdi6.image-filter-center-focus"), parent=parent
        )

        self._recent_images_menu = recent_images_menu

        self.setAllowedAreas(QtCore.Qt.NoDockWidgetArea)
        self.tabs.setTabPosition(QtWidgets.QTabWidget.West)
        self.tabs.currentChanged.connect(self._on_tab_changed)

        self._tab_bar = self.tabs.tabBar()
        self._tab_bar.installEventFilter(self)

        # Widgets
        self._viewers = defaultdict(list)
        self.add_image_viewer()

        # Initialize
        self._update_recent_images_menu()

    def eventFilter(self, watched: QtCore.QObject, event: QtCore.QEvent) -> bool:
        """Tab context menu implementation."""
        if watched == self._tab_bar:
            if event.type() == QtCore.QEvent.ContextMenu:
                pos = event.pos()
                tab_index = self._tab_bar.tabAt(pos)
                tab_widget = self.tabs.widget(tab_index)

                tab_menu = QtWidgets.QMenu(self._tab_bar)
                close_action = tab_menu.addAction(
                    "Close", lambda: self._on_tab_close_requested(tab_index)
                )

                if len(self._viewers.get(type(tab_widget), [])) == 1:
                    # Only enable the action if there is more than one viewer of this
                    # type open.
                    close_action.setEnabled(False)

                tab_menu.popup(self._tab_bar.mapToGlobal(pos))
                return True

        return False

    def load_image(
        self, image_path: Optional[Path] = None, new_tab: bool = False
    ) -> ImageViewer:
        """
        Load an image into a new or existing viewer tab.

        :param image_path: Optional image path to load
        :param new_tab: Whether to load image into a new tab instead of
            the current or first available image viewer.
        :return: Image viewer instance
        """
        if new_tab or not self._viewers.get(ImageViewer):
            image_viewer = self.add_image_viewer()
        else:
            current_viewer = self.tabs.currentWidget()
            if isinstance(current_viewer, ImageViewer):
                image_viewer = current_viewer
            else:
                image_viewer = self._viewers[ImageViewer][0]

        self.tabs.setCurrentWidget(image_viewer)

        if image_path is None or not image_path.is_file():
            image_dir = self._get_image_dir(image_path)

            # Prompt user to choose an image
            image_path_str, sel_filter = QtWidgets.QFileDialog.getOpenFileName(
                self, "Load image", dir=image_dir
            )
            if not image_path_str:
                return image_viewer

            image_path = Path(image_path_str)

        settings.setValue(self.SETTING_IMAGE_DIR, image_path.parent.as_posix())
        self._add_recent_image_path(image_path)

        image_viewer.load_image(image_path=image_path)

        return image_viewer

    def add_image_viewer(self) -> ImageViewer:
        """
        Add a new image viewer tab to the dock.

        :return: Image viewer instance
        """
        image_viewer = ImageViewer()
        self._viewers[ImageViewer].append(image_viewer)

        self.add_tab(
            image_viewer,
            image_viewer.viewer_type_label(),
            image_viewer.viewer_type_icon(),
        )

        return image_viewer

    def update_current_viewer(self) -> None:
        """
        Update the current viewer to reflect the latest config changes.
        """
        viewer = self.tabs.currentWidget()
        viewer.update()

    def reset(self) -> None:
        """
        Reset all viewer tabs to a passthrough state.
        """
        for viewer_type, viewers in self._viewers.items():
            for viewer in viewers:
                viewer.reset()

        TransformManager.reset()

    def _on_tab_changed(self, index: int) -> None:
        """
        Track GL context with the current viewer.
        """
        viewer = self.tabs.widget(index)
        if viewer is not None:
            # Force an update to trigger side effects of a processor change in the
            # wider application.
            viewer.update(force=True)

    def _on_tab_close_requested(self, index: int) -> None:
        """
        Maintain one instance of each viewer type.
        """
        viewer = self.tabs.widget(index)
        viewer_type = type(viewer)

        if len(self._viewers.get(viewer_type, [])) > 1:
            self.tabs.removeTab(index)

            if viewer in self._viewers[viewer_type]:
                self._viewers[viewer_type].remove(viewer)

    def _get_image_dir(self, image_path: Optional[Path] = None) -> str:
        """
        Infer an image load directory from an existing image path or
        settings.
        """
        image_dir = ""
        if image_path is not None:
            image_dir = image_path.parent.as_posix()
        if not image_dir and settings.contains(self.SETTING_IMAGE_DIR):
            image_dir = settings.value(self.SETTING_IMAGE_DIR)
        return image_dir

    def _get_recent_image_paths(self) -> list[Path]:
        """
        Get the 10 most recently loaded image file paths that still
        exist.

        :return: List of image file paths
        """
        recent_images = []

        num_images = settings.beginReadArray(self.SETTING_RECENT_IMAGES)
        for i in range(num_images):
            settings.setArrayIndex(i)
            recent_image_path_str = settings.value(self.SETTING_RECENT_IMAGE_PATH)
            if recent_image_path_str:
                recent_image_path = Path(recent_image_path_str)
                if recent_image_path.is_file():
                    recent_images.append(recent_image_path)
        settings.endArray()

        return recent_images

    def _add_recent_image_path(self, image_path: Path) -> None:
        """
        Add the provided image file path to the top of the recent
        image files list.

        :param image_path: Image file path
        """
        image_paths = self._get_recent_image_paths()
        if image_path in image_paths:
            image_paths.remove(image_path)
        image_paths.insert(0, image_path)

        if len(image_paths) > 10:
            image_paths = image_path[:10]

        settings.beginWriteArray(self.SETTING_RECENT_IMAGES)
        for i, recent_image_path in enumerate(image_paths):
            settings.setArrayIndex(i)
            settings.setValue(
                self.SETTING_RECENT_IMAGE_PATH, recent_image_path.as_posix()
            )
        settings.endArray()

        # Update menu with latest list
        self._update_recent_images_menu()

    def _update_recent_images_menu(self) -> None:
        """Update recent image menu actions."""
        self._recent_images_menu.clear()
        for recent_image_path in self._get_recent_image_paths():
            self._recent_images_menu.addAction(
                recent_image_path.name,
                lambda path=recent_image_path: self.load_image(path),
            )
