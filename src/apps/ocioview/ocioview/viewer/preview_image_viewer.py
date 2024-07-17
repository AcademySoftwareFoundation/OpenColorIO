# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtWidgets

from ..items.display_model import DisplayModel
from ..items.view_model import ViewModel
from ..processor_context import ProcessorContext
from ..ref_space_manager import ReferenceSpaceManager
from ..utils import get_glyph_icon, SignalsBlocked
from ..widgets import CallbackComboBox
from .base_image_viewer import BaseImageViewer


class PreviewImageViewer(BaseImageViewer):
    """Image viewer widget for OCIO UX integration preview."""

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent)

        # Widgets
        self.display_view_label = get_glyph_icon(
            ViewModel.__icon_glyph__, as_widget=True
        )
        self.display_view_label.setToolTip(
            f"{DisplayModel.item_type_label()} / {ViewModel.item_type_label()}"
        )

        self.display_box = CallbackComboBox(
            self._get_displays,
            get_default_item=self._get_default_display,
        )
        self.display_box.setSizeAdjustPolicy(
            QtWidgets.QComboBox.AdjustToContents
        )
        self.display_box.setToolTip(DisplayModel.item_type_label())
        self.display_box.currentIndexChanged[int].connect(
            self._on_display_changed
        )

        self.view_box = CallbackComboBox(
            self._get_views,
            get_default_item=self._get_default_view,
        )
        self.view_box.setSizeAdjustPolicy(QtWidgets.QComboBox.AdjustToContents)
        self.view_box.setToolTip(ViewModel.item_type_label())
        self.view_box.currentIndexChanged[int].connect(self._on_view_changed)

        # Layout
        display_view_layout = QtWidgets.QHBoxLayout()
        display_view_layout.setContentsMargins(0, 0, 0, 0)
        display_view_layout.setSpacing(0)
        display_view_layout.addWidget(self.display_box)
        display_view_layout.addWidget(self.view_box)

        self.io_layout.addWidget(self.display_view_label)
        self.io_layout.addLayout(display_view_layout)
        self.io_layout.setStretch(3, 2)

        # Initialize
        self.update(force=True)

    def update(self, force: bool = False, update_items: bool = True) -> None:
        if update_items:
            self.input_color_space_box.update_items()
            self.display_box.update_items()
            self.view_box.update_items()

        transform = None
        display = self.display()
        view = self.view()

        if display and view:
            # Image plane expects all transforms to be relative to the current
            # config's scene reference space.
            transform = ocio.DisplayViewTransform(
                src=ReferenceSpaceManager.scene_reference_space().getName(),
                display=display,
                view=view,
                direction=ocio.TRANSFORM_DIR_FORWARD,
            )

        self.image_plane.update_ocio_proc(
            proc_context=self._make_processor_context(),
            transform=transform,
            force_update=force,
        )

        super().update()

    def display(self) -> str:
        """Get current OCIO display."""
        return self.display_box.currentText()

    def view(self) -> str:
        """Get current OCIO view."""
        return self.view_box.currentText()

    def _get_displays(self) -> list[str]:
        """Get all active OCIO displays."""
        config = ocio.GetCurrentConfig()
        return list(config.getDisplays())

    def _get_default_display(self) -> str:
        """Get default OCIO display."""
        config = ocio.GetCurrentConfig()
        return config.getDefaultDisplay()

    def _get_views(self) -> list[str]:
        """
        Get all active OCIO views, given the current input color space.
        """
        config = ocio.GetCurrentConfig()
        return config.getViews(self.display(), self.input_color_space())

    def _get_default_view(self) -> str:
        """
        Get default OCIO view, given the current display and input
        color space.
        """
        config = ocio.GetCurrentConfig()
        return config.getDefaultView(self.display(), self.input_color_space())

    def _make_processor_context(self) -> ProcessorContext:
        return ProcessorContext(
            self.input_color_space(),
            ViewModel.__item_type__,
            self.view(),
            ocio.TRANSFORM_DIR_FORWARD,
        )

    @QtCore.Slot(int)
    def _on_display_changed(self, index: int) -> None:
        """Called when the display changes."""
        with SignalsBlocked(self.view_box):
            self.view_box.update_items()
        self._on_view_changed(0)

    @QtCore.Slot(int)
    def _on_view_changed(self, index: int) -> None:
        """Called when the view changes."""
        self.update(update_items=False)
