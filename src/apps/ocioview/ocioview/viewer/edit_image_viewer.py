# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

from typing import Optional, Type

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from ..processor_context import ProcessorContext
from ..transform_manager import TransformManager
from ..utils import get_glyph_icon, SignalsBlocked
from ..widgets import ComboBox
from .base_image_viewer import BaseImageViewer


class EditImageViewer(BaseImageViewer):
    """Image viewer widget for OCIO config editing."""

    PASSTHROUGH = "passthrough"
    PASSTHROUGH_LABEL = BaseImageViewer.FMT_GRAY_LABEL.format(
        v=f"{PASSTHROUGH}:"
    )

    ROLE_SLOT = QtCore.Qt.UserRole + 1
    ROLE_ITEM_TYPE = QtCore.Qt.UserRole + 2
    ROLE_ITEM_NAME = QtCore.Qt.UserRole + 3

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent)

        self._tf_subscription_slot = -1
        self._tf_fwd = None
        self._tf_inv = None

        # Widgets
        self.tf_label = get_glyph_icon("mdi6.export", as_widget=True)
        self.tf_box = ComboBox()
        self.tf_box.setToolTip("Output transform")

        self._tf_direction_forward_icon = get_glyph_icon("mdi6.layers-plus")
        self._tf_direction_inverse_icon = get_glyph_icon("mdi6.layers-minus")

        self.tf_direction_button = QtWidgets.QPushButton()
        self.tf_direction_button.setCheckable(True)
        self.tf_direction_button.setChecked(False)
        self.tf_direction_button.setIcon(self._tf_direction_forward_icon)
        self.tf_direction_button.setToolTip("Transform direction: Forward")

        self.output_tf_direction_label = QtWidgets.QLabel("+")

        # Layout
        tf_layout = QtWidgets.QHBoxLayout()
        tf_layout.setContentsMargins(0, 0, 0, 0)
        tf_layout.setSpacing(0)
        tf_layout.addWidget(self.tf_box)
        tf_layout.setStretch(0, 1)
        tf_layout.addWidget(self.tf_direction_button)

        self.io_layout.addWidget(self.tf_label)
        self.io_layout.addLayout(tf_layout)
        self.io_layout.setStretch(3, 1)

        self.inspect_layout.addWidget(
            self.output_tf_direction_label, 1, 5, QtCore.Qt.AlignRight
        )

        # Connect signals/slots
        self.image_plane.tf_subscription_requested.connect(
            self._on_tf_subscription_requested
        )
        self.tf_box.currentIndexChanged[int].connect(
            self._on_transform_changed
        )
        self.tf_direction_button.clicked[bool].connect(
            self._on_inverse_check_clicked
        )

        # Initialize
        TransformManager.subscribe_to_transform_menu(
            self._on_transform_menu_changed
        )
        TransformManager.subscribe_to_transform_subscription_init(
            self._on_transform_subscription_init
        )
        self.update(force=True)

    def update(self, force: bool = False, update_items: bool = True) -> None:
        self.image_plane.update_ocio_proc(
            proc_context=self._make_processor_context(), force_update=force
        )
        super().update()

    def reset(self) -> None:
        super().reset()

        # Update widgets to match image plane
        with SignalsBlocked(self):
            self.set_transform_direction(ocio.TRANSFORM_DIR_FORWARD)

    def transform(self) -> Optional[ocio.Transform]:
        """Get current OCIO transform."""
        return self.image_plane.transform()

    def set_transform(
        self,
        slot: int,
        transform_fwd: Optional[ocio.Transform],
        transform_inv: Optional[ocio.Transform],
    ) -> None:
        """
        Update main OCIO transform for the viewing pipeline, to be
        applied from the current config's scene reference space.

        :param slot: Transform subscription slot
        :param transform_fwd: Forward transform
        :param transform_inv: Inverse transform
        """
        tf_direction = self.transform_direction()

        if (
            slot != self._tf_subscription_slot
            or (
                transform_fwd is None
                and tf_direction == ocio.TRANSFORM_DIR_FORWARD
            )
            or (
                transform_inv is None
                and tf_direction == ocio.TRANSFORM_DIR_INVERSE
            )
        ):
            return

        self._tf_fwd = transform_fwd
        self._tf_inv = transform_inv

        self.image_plane.update_ocio_proc(
            proc_context=self._make_processor_context(),
            transform=self._tf_inv
            if tf_direction == ocio.TRANSFORM_DIR_INVERSE
            else self._tf_fwd,
        )

    def clear_transform(self) -> None:
        """
        Clear current OCIO transform, passing through the input image.
        """
        self._tf_subscription_slot = -1
        self._tf_fwd = None
        self._tf_inv = None

        if self.tf_box.currentIndex() != 0:
            with SignalsBlocked(self.tf_box):
                self.tf_box.setCurrentIndex(0)

        self.image_plane.clear_transform()

    def transform_item_type(self) -> Type | None:
        """Get transform source config item type."""
        return self.tf_box.currentData(role=self.ROLE_ITEM_TYPE)

    def transform_item_name(self) -> str | None:
        """Get transform source config item name."""
        return self.tf_box.currentData(role=self.ROLE_ITEM_NAME)

    def transform_direction(self) -> ocio.TransformDirection:
        """Get transform direction being viewed."""
        return (
            ocio.TRANSFORM_DIR_INVERSE
            if self.tf_direction_button.isChecked()
            else ocio.TRANSFORM_DIR_FORWARD
        )

    def set_transform_direction(
        self, direction: ocio.TransformDirection
    ) -> None:
        """
        :param direction: Set the transform direction to be viewed
        """
        self.tf_direction_button.setChecked(
            direction == ocio.TRANSFORM_DIR_INVERSE
        )

    def _make_processor_context(self) -> ProcessorContext:
        return ProcessorContext(
            self.input_color_space(),
            self.transform_item_type(),
            self.transform_item_name(),
            self.transform_direction(),
        )

    def _on_transform_menu_changed(
        self, menu_items: list[tuple[int, str, QtGui.QIcon]]
    ) -> None:
        """
        Called to refresh transform menu items, and either reselect the
        existing subscription, or deselect any subscription.
        """
        target_index = -1
        current_slot = -1
        if self.tf_box.count():
            current_slot = self.tf_box.currentData(role=self.ROLE_SLOT)

        with SignalsBlocked(self.tf_box):
            self.tf_box.clear()

            # The first item is always no transform
            self.tf_box.addItem(self.PASSTHROUGH)
            self.tf_box.setItemData(0, -1, role=self.ROLE_SLOT)

            for i, (
                slot,
                item_label,
                item_type,
                item_name,
                slot_icon,
            ) in enumerate(menu_items):
                index = i + 1
                self.tf_box.addItem(slot_icon, item_label)
                self.tf_box.setItemData(index, slot, role=self.ROLE_SLOT)
                self.tf_box.setItemData(
                    index, item_type, role=self.ROLE_ITEM_TYPE
                )
                self.tf_box.setItemData(
                    index, item_name, role=self.ROLE_ITEM_NAME
                )
                if slot == current_slot:
                    target_index = index  # Offset for "Passthrough" item

            # Restore previous item?
            if target_index != -1:
                self.tf_box.setCurrentIndex(target_index)

        # Switch to "Passthrough" if previous slot not found
        if target_index == -1 and self.tf_box.count():
            with SignalsBlocked(self.tf_box):
                self.tf_box.setCurrentIndex(0)

            # Force update transform
            self._on_transform_changed(0)

    def _on_transform_subscription_init(self, slot: int) -> None:
        """
        If this viewer is not subscribed to a specific transform
        subscription slot, subscribe to the first slot to receive a
        transform subscription.

        :param slot: Transform subscription slot
        """
        if self._tf_subscription_slot == -1:
            index = self.tf_box.findData(slot, role=self.ROLE_SLOT)
            if index != -1:
                self.tf_box.setCurrentIndex(index)

    @QtCore.Slot(int)
    def _on_transform_changed(self, index: int) -> None:
        if index == 0:
            TransformManager.unsubscribe_from_all_transforms(
                self.set_transform
            )
            self.clear_transform()
        else:
            self._tf_subscription_slot = self.tf_box.currentData(
                role=self.ROLE_SLOT
            )
            TransformManager.subscribe_to_transforms_at(
                self._tf_subscription_slot, self.set_transform
            )

    @QtCore.Slot(int)
    def _on_tf_subscription_requested(self, slot: int) -> None:
        # If the requested slot does not have a subscription, "Passthrough" will
        # be selected.
        self.tf_box.setCurrentIndex(
            max(0, self.tf_box.findData(slot, role=self.ROLE_SLOT))
        )

    @QtCore.Slot(bool)
    def _on_inverse_check_clicked(self, checked: bool) -> None:
        self.set_transform(
            self._tf_subscription_slot, self._tf_fwd, self._tf_inv
        )
        if self.tf_direction_button.isChecked():
            self.tf_direction_button.setIcon(self._tf_direction_inverse_icon)
            self.tf_direction_button.setToolTip("Transform direction: Inverse")
            # Use 'minus' character to match the width of "+"
            self.output_tf_direction_label.setText("\u2212")
        else:
            self.tf_direction_button.setIcon(self._tf_direction_forward_icon)
            self.tf_direction_button.setToolTip("Transform direction: Forward")
            self.output_tf_direction_label.setText("+")
