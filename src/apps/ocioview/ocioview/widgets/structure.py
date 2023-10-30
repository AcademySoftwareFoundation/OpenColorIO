# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from pathlib import Path
from typing import Optional, Union

from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import ICON_SIZE_BUTTON, BORDER_COLOR_ROLE
from ..style import apply_top_tool_bar_style
from ..utils import get_icon


class DockTitleBar(QtWidgets.QFrame):
    """Dock widget title bar widget with icon."""

    def __init__(
        self, title: str, icon: QtGui.QIcon, parent: Optional[QtCore.QObject] = None
    ):
        """
        :param title: Title text
        :param icon: Dock icon
        """
        super().__init__(parent=parent)

        self.setFrameShape(QtWidgets.QFrame.StyledPanel)
        self.setObjectName("dock_title_bar")
        apply_top_tool_bar_style(
            self, bg_color_role=None, border_color_role=BORDER_COLOR_ROLE
        )

        # Widgets
        self.icon = QtWidgets.QLabel()
        self.icon.setPixmap(icon.pixmap(ICON_SIZE_BUTTON))
        self.title = QtWidgets.QLabel(title)

        # Layout
        inner_layout = QtWidgets.QHBoxLayout()
        inner_layout.setContentsMargins(10, 8, 10, 8)
        inner_layout.setSpacing(5)
        inner_layout.addWidget(self.icon)
        inner_layout.addWidget(self.title)
        inner_layout.addStretch()

        inner_frame = QtWidgets.QFrame()
        inner_frame.setFrameShape(QtWidgets.QFrame.StyledPanel)
        inner_frame.setObjectName("dock_title_bar__inner_frame")
        apply_top_tool_bar_style(inner_frame)
        inner_frame.setLayout(inner_layout)

        layout = QtWidgets.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(inner_frame)

        self.setLayout(layout)


class TabbedDockWidget(QtWidgets.QDockWidget):
    """
    Dockable tab widget with tab icons that are always oriented
    upward.
    """

    # Cached tab icon paths
    _tab_icons = {}

    def __init__(
        self, title: str, icon: QtGui.QIcon, parent: Optional[QtCore.QObject] = None
    ):
        """
        :param title: Title text
        :param icon: Dock icon
        """
        super().__init__(parent=parent)
        self.setTitleBarWidget(DockTitleBar(title, icon))
        self.setFeatures(
            QtWidgets.QDockWidget.DockWidgetMovable
            | QtWidgets.QDockWidget.DockWidgetFloatable
        )

        self._prev_index = 0

        # Widgets
        self.tabs = QtWidgets.QTabWidget()
        self.setWidget(self.tabs)

        # Connections
        self.dockLocationChanged.connect(self._on_dock_location_changed)
        self.tabs.currentChanged.connect(self._on_current_changed)

    def add_tab(
        self,
        widget: QtWidgets.QWidget,
        name: str,
        icon_or_path: Union[Path, QtGui.QIcon],
        tool_tip: Optional[str] = None,
    ) -> None:
        """
        Add widget as tab with icon and tool tip.

        :param widget: Widget to add as new tab
        :param name: Tab name
        :param icon_or_path: Icon file path or QIcon instance
        :param tool_tip: Optional tab tooltip. If unspecified, a tooltip
            will be determined from the icon name.
        """
        # Store original icon for rotation adjustments
        if isinstance(icon_or_path, Path):
            icon = get_icon(icon_or_path)
        else:
            icon = icon_or_path

        self._tab_icons[id(widget)] = icon

        # Add new tab, with icon oriented upward
        tab_pos = self.tabs.tabPosition()
        upright_icon = self._rotate_icon(icon, tab_pos)

        tab_idx = self.tabs.addTab(widget, upright_icon, "")
        self.tabs.setTabToolTip(tab_idx, tool_tip or name)

    def _rotate_icon(
        self, icon: QtGui.QIcon, tab_pos: QtWidgets.QTabWidget.TabPosition
    ) -> QtGui.QIcon:
        """
        Rotate icon to be oriented upward for the given tab position.

        :param icon: Icon to rotate
        :param tab_pos: Tab position to orient icon for
        :return: Rotated icon
        """
        icon_rot = {
            QtWidgets.QTabWidget.East: -90,
            QtWidgets.QTabWidget.West: 90,
        }.get(tab_pos, 0)

        xform = QtGui.QTransform()
        xform.rotate(icon_rot)

        pixmap = icon.pixmap(ICON_SIZE_BUTTON)
        pixmap = pixmap.transformed(xform)

        return QtGui.QIcon(pixmap)

    @QtCore.Slot(QtCore.Qt.DockWidgetArea)
    def _on_dock_location_changed(self, area: QtCore.Qt.DockWidgetArea) -> None:
        """
        Adjust tab icons to always orient upward on dock area move.
        """
        if area == QtCore.Qt.LeftDockWidgetArea:
            tab_pos = QtWidgets.QTabWidget.East
        else:
            tab_pos = QtWidgets.QTabWidget.West

        self.tabs.setTabPosition(tab_pos)

        # Rotate tab icons so they are always oriented upward
        for tab_idx in range(self.tabs.count()):
            widget = self.tabs.widget(tab_idx)

            # Get previously stored, un-rotated, icon from widget
            icon = self._tab_icons.get(id(widget))
            if icon is not None:
                upright_icon = self._rotate_icon(icon, tab_pos)
                self.tabs.setTabIcon(tab_idx, upright_icon)

    def _on_current_changed(self, index: int) -> None:
        prev_widget = self.tabs.widget(self._prev_index)
        next_widget = self.tabs.widget(index)

        # If previous and next tabs both have splitters of the same size, make their
        # sizes match.
        if hasattr(prev_widget, "splitter") and hasattr(
            next_widget, "set_splitter_sizes"
        ):
            next_widget.set_splitter_sizes(prev_widget.splitter.sizes())

        self._prev_index = index


class ExpandingStackedWidget(QtWidgets.QStackedWidget):
    """
    Stacked widget that adjusts its height to the current widget,
    rather than match the largest widget in the stack.
    """

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)
        self.currentChanged.connect(self._on_current_changed)

    def addWidget(self, widget: QtWidgets.QWidget) -> None:
        """
        :param widget: Widget to add at the back of the stack and make
            current.
        """
        super().addWidget(widget)
        self._on_current_changed(self.currentIndex())

    @QtCore.Slot(int)
    def _on_current_changed(self, index: int) -> None:
        """
        Toggle widget size policy to ignore invisible widget sizes.
        """
        for i in range(self.count()):
            widget = self.widget(i)
            if i == index:
                widget.setSizePolicy(
                    QtWidgets.QSizePolicy.Expanding,
                    QtWidgets.QSizePolicy.MinimumExpanding,
                )
            else:
                widget.setSizePolicy(
                    QtWidgets.QSizePolicy.Ignored, QtWidgets.QSizePolicy.Ignored
                )
