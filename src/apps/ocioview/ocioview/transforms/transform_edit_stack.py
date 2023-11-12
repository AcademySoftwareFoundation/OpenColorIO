# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from functools import partial
from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

# Register all transform edit types
from .. import transforms

from ..constants import ICON_SIZE_BUTTON, MARGIN_WIDTH
from ..utils import get_glyph_icon
from .transform_edit import BaseTransformEdit
from .transform_edit_factory import TransformEditFactory
from .utils import ravel_transform


class TransformEditStack(QtWidgets.QWidget):
    """
    Widget that composes one or more transform edits to reflect the
    transforms for an object in the config in a single direction.
    """

    edited = QtCore.Signal()

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Fits widest transform edit (Exponent[WithLinear]Transform)
        self.setMinimumWidth(500)

        # Widgets
        self.tf_menu = QtWidgets.QMenu()
        for tf_type in TransformEditFactory.transform_types():
            tf_edit_type = TransformEditFactory.transform_edit_type(tf_type)
            self.tf_menu.addAction(
                tf_edit_type.transform_type_icon(),
                tf_edit_type.transform_type_label(),
                lambda t=tf_type: self._create_transform_edit(t),
            )

        self.add_tf_button = QtWidgets.QToolButton()
        self.add_tf_button.setText(" ")
        self.add_tf_button.setIcon(get_glyph_icon("ph.plus"))
        self.add_tf_button.setPopupMode(QtWidgets.QToolButton.InstantPopup)
        self.add_tf_button.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)
        self.add_tf_button.setMenu(self.tf_menu)

        self._start_collapsed_action = QtGui.QAction("Start Collapsed")
        self._start_collapsed_action.setCheckable(True)
        self._start_collapsed_action.triggered[bool].connect(
            self._on_start_collapsed_changed
        )

        self.settings_menu = QtWidgets.QMenu()
        self.settings_menu.addAction(self._start_collapsed_action)

        settings_button = QtWidgets.QToolButton()
        settings_button.setText(" ")
        settings_button.setIcon(get_glyph_icon("ph.gear"))
        settings_button.setPopupMode(QtWidgets.QToolButton.InstantPopup)
        settings_button.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)
        settings_button.setMenu(self.settings_menu)

        tool_bar = QtWidgets.QToolBar()
        tool_bar.setStyleSheet(
            "QToolButton::menu-indicator {"
            "    subcontrol-position: center right;"
            "    right: 7px;"
            "}"
        )
        tool_bar.setIconSize(ICON_SIZE_BUTTON)
        tool_bar.addWidget(self.add_tf_button)
        tool_bar.addWidget(settings_button)

        self.tf_info_label = QtWidgets.QLabel("")

        self.tf_layout = QtWidgets.QVBoxLayout()
        self.tf_layout.addStretch()
        tf_frame = QtWidgets.QWidget()
        tf_frame.setLayout(self.tf_layout)

        tf_scroll_area = QtWidgets.QScrollArea()
        tf_scroll_area.setObjectName("transform_edit_stack_scroll_area")
        tf_scroll_area.setSizePolicy(
            QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.MinimumExpanding
        )
        tf_scroll_area.setWidgetResizable(True)
        tf_scroll_area.setStyleSheet(
            f"QScrollArea#transform_edit_stack_scroll_area {{"
            f"    border: none;"
            f"    border-top: 1px solid "
            f"        {self.palette().color(QtGui.QPalette.Dark).name()};"
            f"}}"
        )
        tf_scroll_area.setWidget(tf_frame)

        # Layout
        top_layout = QtWidgets.QHBoxLayout()
        top_layout.setContentsMargins(0, 0, MARGIN_WIDTH, 0)
        top_layout.addWidget(tool_bar)
        top_layout.addStretch()
        top_layout.addWidget(self.tf_info_label)
        layout = QtWidgets.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addLayout(top_layout)
        layout.addWidget(tf_scroll_area)

        self.setLayout(layout)

        # Initialize
        self._update_transform_info_label()

    def reset(self) -> None:
        """
        Clear all transforms.
        """
        self.set_transform(None)

    def transform(self) -> Optional[ocio.Transform]:
        """
        Compose the stack into a single transform and return it. If
        there are multiple transform edits, this will be a group
        transform, otherwise it will be a single transform or None, for
        one or zero transform edits respectively.

        :return: Composed transform
        """
        tfs = []
        for i in range(self.transform_count()):
            item = self.tf_layout.itemAt(i)
            widget = item.widget()
            if widget:
                if isinstance(widget, BaseTransformEdit):
                    tfs.append(widget.transform())

        tf_count = len(tfs)
        if not tf_count:
            return None
        elif tf_count == 1:
            return tfs[0]
        else:
            # Adding transforms in the constructor will validate all transforms, which
            # is avoided here since transforms may be in an intermediate state while
            # being edited.
            group_tf = ocio.GroupTransform()
            for tf in tfs:
                group_tf.appendTransform(tf)
            return group_tf

    def set_transform(self, transform: Optional[ocio.Transform]) -> None:
        """
        Reinitialize stack from a transform. Group transforms will be
        raveled recursively into a flattened list of transform edits.

        :param transform: Transform to decompose
        """
        start_collapsed = self._start_collapsed_action.isChecked()

        if transform is not None:
            tfs = ravel_transform(transform)
        else:
            tfs = []

        # Do transforms match current widgets?
        tf_count = len(tfs)
        tf_edits = self.transform_edits()

        if tf_count == len(tf_edits):
            if [tf.__class__ for tf in tfs] == [
                tf_edit.__tf_type__ for tf_edit in tf_edits
            ]:
                # Update transform widgets
                for i in range(tf_count):
                    tf_edits[i].update_from_transform(tfs[i])
                return

        # Rebuild transforms
        self._clear_transform_layout()

        for tf in tfs:
            tf_edit = TransformEditFactory.from_transform(tf)
            tf_edit.set_collapsed(start_collapsed)
            self._connect_signals(tf_edit)
            self.tf_layout.addWidget(tf_edit)
        self.tf_layout.addStretch()

        self._on_transforms_changed()

    def transform_count(self) -> int:
        """
        :return: Number of transforms in stack
        """
        # -1 to account for spacer item beneath transform edits
        return self.tf_layout.count() - 1

    def transform_edits(self) -> list[BaseTransformEdit]:
        """
        :return: list of transform edits in stack
        """
        tf_edits = []
        for i in range(self.transform_count()):
            item = self.tf_layout.itemAt(i)
            widget = item.widget()
            if widget and isinstance(widget, BaseTransformEdit):
                tf_edits.append(widget)
        return tf_edits

    def _create_transform_edit(self, transform_type: type) -> None:
        """
        Create a new transform edit from a transform type.

        :param transform_type: Transform class
        """
        tf_edit = TransformEditFactory.from_transform_type(transform_type)
        tf_edit.set_collapsed(self._start_collapsed_action.isChecked())
        self._connect_signals(tf_edit)
        self._insert_transform_edit(self.transform_count(), tf_edit)

    def _insert_transform_edit(
        self, index: int, transform_edit: BaseTransformEdit
    ) -> None:
        """
        Insert transform edit in the stack at the specified index.

        :param index: Stack index
        :param transform_edit: Transform edit to insert
        """
        self.tf_layout.insertWidget(index, transform_edit)
        transform_edit.show()

        self._on_transforms_changed()

    def _pop_transform_edit(self, transform_edit: BaseTransformEdit) -> int:
        """
        Remove the specified transform edit from the stack and return
        its index.

        :param transform_edit: Transform edit to remove
        :return: Stack index
        """
        for i in range(self.tf_layout.count()):
            item = self.tf_layout.itemAt(i)
            widget = item.widget()

            if widget == transform_edit:
                # Hide the widget before unparenting to prevent it from being
                # raised in its own window.
                widget.hide()
                widget.setParent(None)
                return i

        # If widget was not in layout, assume it was at the bottom
        return self.transform_count()

    def _clear_transform_layout(self) -> None:
        """
        Remove all transform edits from stack.
        """
        for i in reversed(range(self.tf_layout.count())):
            item = self.tf_layout.takeAt(i)
            widget = item.widget()

            if widget:
                # Hide the widget before unparenting to prevent it from being
                # raised in its own window.
                widget.hide()
                widget.setParent(None)
                widget.deleteLater()

        self._on_transforms_changed()

    @QtCore.Slot(QtWidgets.QWidget)
    def _on_transform_edit_deleted(self, transform_edit: BaseTransformEdit) -> None:
        """
        Remove and delete the specified transform edit.

        :param transform_edit: Transform edit to remove
        """
        self._pop_transform_edit(transform_edit)
        transform_edit.deleteLater()

        self._on_transforms_changed()

    @QtCore.Slot(QtWidgets.QWidget)
    def _on_transform_edit_moved(
        self, move: int, transform_edit: BaseTransformEdit
    ) -> None:
        """
        Offset the specified transform edit index by a signed 'move'
        value.

        :param move: Signed index offset
        :param transform_edit: Transform edit to move
        """
        tf_count = self.transform_count()

        # Can a transform be moved?
        if tf_count <= 1:
            return

        prev_index = self._pop_transform_edit(transform_edit)

        self._insert_transform_edit(
            # Clamp between negative index (which will not insert the widget),
            # and spacer item index (last item).
            min(tf_count - 1, max(0, prev_index + move)),
            transform_edit,
        )

    @QtCore.Slot(bool)
    def _on_start_collapsed_changed(self, checked: bool) -> None:
        """
        Update collapsed state upon changing the "Start Collapsed"
        setting.
        """
        for tf_edit in self.transform_edits():
            tf_edit.set_collapsed(checked)

    def _on_transforms_changed(self) -> None:
        """
        Notify the application that transforms in the stack have
        changed.
        """
        self.edited.emit()
        self._update_transform_info_label()

    def _connect_signals(self, transform_edit: BaseTransformEdit) -> None:
        """
        Connect transform edit signals to stack. This facilitates
        transform edit move and delete buttons, and notifications to
        the application of changed transform parameters.

        :param transform_edit: Transform edit to connect
        """
        transform_edit.edited.connect(self.edited.emit)
        transform_edit.deleted.connect(self._on_transform_edit_deleted)
        transform_edit.moved_up.connect(partial(self._on_transform_edit_moved, -1))
        transform_edit.moved_down.connect(partial(self._on_transform_edit_moved, 1))

    def _update_transform_info_label(self) -> None:
        """
        Update transform info label to list the current transform
        count.
        """
        tf_count = self.transform_count()
        self.tf_info_label.setText(
            f"<i>{tf_count} transform{'s' if tf_count != 1 else ''}</i>"
        )
