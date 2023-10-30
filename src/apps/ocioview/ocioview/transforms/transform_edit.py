# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

from functools import partial
from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import ICON_SIZE_ITEM, BORDER_COLOR_ROLE
from ..style import apply_top_tool_bar_style, apply_widget_with_top_tool_bar_style
from ..utils import get_glyph_icon, item_type_label
from ..widgets import EnumComboBox, FormLayout


class BaseTransformEdit(QtWidgets.QFrame):
    """
    Base widget for editing an OCIO transform instance.
    """

    # Transform type this widget edits, set on registration with
    # TransformEditFactory.
    __tf_type__: type = ocio.Transform

    # Transform type label for use in GUI components, generated on first call to
    # ``transform_type_label()``.
    __tf_type_label__: str = None

    # QtAwesome glyph name to use for this transform type's icon
    __icon_glyph__: str = None

    # Transform type icon, loaded on first call to ``transform_type_icon()``.
    __icon__: QtGui.QIcon = None

    edited = QtCore.Signal()
    moved_up = QtCore.Signal(QtWidgets.QWidget)
    moved_down = QtCore.Signal(QtWidgets.QWidget)
    deleted = QtCore.Signal(QtWidgets.QWidget)

    @classmethod
    def transform_type_icon(cls) -> QtGui.QIcon:
        """
        :return: Transform type icon
        """
        if cls.__icon__ is None:
            cls.__icon__ = get_glyph_icon(cls.__icon_glyph__)
        return cls.__icon__

    @classmethod
    def transform_type_label(cls) -> str:
        """
        :return: Friendly type name
        """
        if cls.__tf_type_label__ is None:
            # Remove trailing "Transform" token
            cls.__tf_type_label__ = item_type_label(cls.__tf_type__).rsplit(" ", 1)[0]
        return cls.__tf_type_label__

    @classmethod
    def create_transform(cls, *args, **kwargs) -> ocio.Transform:
        """
        Create a new transform with passthrough constructor
        args/kwargs.

        :return: Transform instance
        """
        return cls.__tf_type__(*args, **kwargs)

    @classmethod
    def from_transform(cls, transform: ocio.Transform) -> BaseTransformEdit:
        """
        Create and populate a transform edit from an existing transform
        instance.

        :param transform: Transform to create edit widget for
        :return: Transform edit
        """
        tf_edit = cls()
        tf_edit.update_from_transform(transform)
        return tf_edit

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        palette = self.palette()

        self.setFrameShape(QtWidgets.QFrame.StyledPanel)
        self.setObjectName("transform_edit")
        apply_widget_with_top_tool_bar_style(self)

        # Widgets
        self._expand_icon = get_glyph_icon("ph.caret-right")
        self._collapse_icon = get_glyph_icon("ph.caret-down")

        self.icon_label = None
        if self.__icon_glyph__ is not None:
            self.icon_label = get_glyph_icon(self.__icon_glyph__, as_widget=True)

        self.expand_button = QtWidgets.QToolButton()
        self.expand_button.setIcon(self._collapse_icon)
        self.expand_button.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
        self.expand_button.released.connect(self._on_expand_button_released)

        self.move_up_button = QtWidgets.QToolButton()
        self.move_up_button.setIcon(get_glyph_icon("ph.arrow-up"))
        self.move_up_button.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
        self.move_up_button.released.connect(partial(self.moved_up.emit, self))

        self.move_down_button = QtWidgets.QToolButton()
        self.move_down_button.setIcon(get_glyph_icon("ph.arrow-down"))
        self.move_down_button.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
        self.move_down_button.released.connect(partial(self.moved_down.emit, self))

        self.delete_button = QtWidgets.QToolButton()
        self.delete_button.setIcon(get_glyph_icon("ph.x"))
        self.delete_button.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
        self.delete_button.released.connect(partial(self.deleted.emit, self))

        tool_bar_left = QtWidgets.QToolBar()
        tool_bar_left.setContentsMargins(0, 0, 0, 0)
        tool_bar_left.setIconSize(ICON_SIZE_ITEM)
        if self.icon_label is not None:
            tool_bar_left.addWidget(self.icon_label)
        tool_bar_left.addWidget(self.expand_button)

        tool_bar_right = QtWidgets.QToolBar()
        tool_bar_right.setContentsMargins(0, 0, 0, 0)
        tool_bar_right.setIconSize(ICON_SIZE_ITEM)
        tool_bar_right.addWidget(self.move_up_button)
        tool_bar_right.addWidget(self.move_down_button)
        tool_bar_right.addWidget(self.delete_button)

        self.direction_combo = EnumComboBox(ocio.TransformDirection)
        self.direction_combo.currentIndexChanged.connect(self._on_edit)

        # Layout
        self.tf_layout = FormLayout()
        self.tf_layout.setContentsMargins(12, 12, 12, 12)
        self.tf_layout.setLabelAlignment(QtCore.Qt.AlignRight)
        self.tf_layout.addRow("Direction", self.direction_combo)

        self._tf_frame = QtWidgets.QFrame()
        self._tf_frame.setObjectName("transform_edit__tf_frame")
        self._tf_frame.setStyleSheet(
            f"QFrame#transform_edit__tf_frame {{"
            f"    border-top: 1px solid {palette.color(BORDER_COLOR_ROLE).name()};"
            f"}}"
        )
        self._tf_frame.setLayout(self.tf_layout)

        header_layout = QtWidgets.QHBoxLayout()
        header_layout.setContentsMargins(0, 0, 0, 0)
        header_layout.setSpacing(0)
        header_layout.addWidget(tool_bar_left)
        header_layout.addWidget(QtWidgets.QLabel(self.transform_type_label()))
        header_layout.addStretch()
        header_layout.addWidget(tool_bar_right)

        self._header_frame = QtWidgets.QFrame()
        self._header_frame.setObjectName("transform_edit__header_frame")
        self._header_frame.setLayout(header_layout)

        layout = QtWidgets.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self._header_frame)
        layout.addWidget(self._tf_frame)

        self.setLayout(layout)

        # Initialize
        self._update_state()

    def transform(self) -> ocio.Transform:
        """
        Create a new transform from the current widget values.
        Subclasses must extend this method to account for all transform
        parameters.

        :return: New transform
        """
        transform = self.create_transform()
        transform.setDirection(self.direction_combo.member())
        return transform

    def update_from_transform(self, transform: ocio.Transform) -> None:
        """
        Update an existing transform from the current widget values.
        Subclasses must extend this method to account for all transform
        parameters.

        :param transform: Transform to update
        """
        self.direction_combo.set_member(transform.getDirection())

    def update_from_config(self) -> None:
        """
        Subclasses must update widget state from the current config,
        which could change widget options or data (e.g. available color
        spaces).
        """
        pass

    def set_collapsed(self, collapsed: bool) -> None:
        """
        Set the widget's collapsed state, to show or hide the
        transform parameter widgets.

        :param collapsed: Collapsed state
        """
        self._tf_frame.setHidden(collapsed)
        self._update_state()

    def _on_edit(self, *args, **kwargs) -> None:
        """
        Subclasses must connect all widget modified signals to this
        slot to notify the application of changes affecting the current
        config.
        """
        self.edited.emit()

    def _on_expand_button_released(self) -> None:
        """
        Hide transform parameter widgets to toggle the widget's
        collapsed state.
        """
        if self._tf_frame.isHidden():
            self._tf_frame.setHidden(False)
        else:
            self._tf_frame.setHidden(True)
        self._update_state()

    def _update_state(self) -> None:
        """
        Restyle widget to toggle its collapsed state.
        """
        if self._tf_frame.isHidden():
            self.expand_button.setIcon(self._expand_icon)
            apply_top_tool_bar_style(self._header_frame, border_bottom_radius=3)
        else:
            self.expand_button.setIcon(self._collapse_icon)
            apply_top_tool_bar_style(self._header_frame, border_bottom_radius=0)
