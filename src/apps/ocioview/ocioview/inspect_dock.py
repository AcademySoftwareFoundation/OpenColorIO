# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Optional

from PySide6 import QtCore, QtWidgets

from .inspect import (
    ChromaticitiesInspector,
    CodeInspector,
    CurveInspector,
    LogInspector,
)
from .utils import get_glyph_icon
from .widgets.structure import TabbedDockWidget


class InspectDock(TabbedDockWidget):
    """
    Dockable widget for inspecting and visualizing config and color
    transform data.
    """

    def __init__(
        self,
        corner_widget: Optional[QtWidgets.QWidget] = None,
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param corner_widget: Optional widget to place on the right
            side of the dock title bar.
        """
        super().__init__(
            "Inspect",
            get_glyph_icon("mdi6.dna"),
            corner_widget=corner_widget,
            parent=parent,
        )

        self.setAllowedAreas(
            QtCore.Qt.BottomDockWidgetArea | QtCore.Qt.TopDockWidgetArea
        )
        self.tabs.setTabPosition(QtWidgets.QTabWidget.West)

        # Widgets
        self.chromaticities_inspector = ChromaticitiesInspector()
        self.curve_inspector = CurveInspector()
        self.code_inspector = CodeInspector()
        self.log_inspector = LogInspector()

        # Layout
        self.add_tab(
            self.chromaticities_inspector,
            self.chromaticities_inspector.label(),
            self.chromaticities_inspector.icon(),
        )
        self.add_tab(
            self.curve_inspector,
            self.curve_inspector.label(),
            self.curve_inspector.icon(),
        )
        self.add_tab(
            self.code_inspector,
            self.code_inspector.label(),
            self.code_inspector.icon(),
        )
        self.add_tab(
            self.log_inspector,
            self.log_inspector.label(),
            self.log_inspector.icon(),
        )

    def reset(self) -> None:
        """Reset data for all inspectors, except the log."""
        self.chromaticities_inspector.reset()
        self.curve_inspector.reset()
        self.code_inspector.reset()
