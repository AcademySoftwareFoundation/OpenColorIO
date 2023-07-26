# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from collections import defaultdict
from pathlib import Path
from typing import Optional

import PyOpenColorIO as ocio
from PySide2 import QtCore, QtWidgets

from .transform_manager import TransformManager
from .utils import get_glyph_icon
from .viewer import ImageViewer
from .widgets.structure import TabbedDockWidget


class ViewerDock(TabbedDockWidget):
    """
    Dockable widget for viewing color transforms on images.
    """

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(
            "Viewer", get_glyph_icon("mdi6.image-filter-center-focus"), parent=parent
        )

        self.setAllowedAreas(QtCore.Qt.NoDockWidgetArea)
        self.tabs.setTabPosition(QtWidgets.QTabWidget.West)
        self.tabs.currentChanged.connect(self._on_tab_changed)

        self._tab_bar = self.tabs.tabBar()
        self._tab_bar.installEventFilter(self)

        # Widgets
        self._viewers = defaultdict(list)
        self.add_image_viewer()

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
            viewer.update()

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
