# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from functools import partial
from typing import Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtWidgets

from ..config_cache import ConfigCache
from ..transform_manager import TransformManager
from ..widgets import (
    CallbackComboBox,
    ExpandingStackedWidget,
    FormLayout,
    ItemModelListWidget,
    LineEdit,
)
from .config_item_edit import BaseConfigItemParamEdit, BaseConfigItemEdit
from .display_model import DisplayModel
from .view_model import ViewType, ViewModel


class ViewParamEdit(BaseConfigItemParamEdit):
    """
    Widget for editing the parameters for a display/view pair.
    """

    __model_type__ = ViewModel
    __has_transforms__ = False

    VIEW_LAYERS = [
        None,
        ViewType.VIEW_SHARED,
        ViewType.VIEW_DISPLAY,
        ViewType.VIEW_SCENE,
    ]

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        self.display_model = DisplayModel()

        # Keep track of existing mapper connections to prevent duplicate signal/slot
        # connections.
        self._connected = {}

        # Build stack widget layers
        self._param_stack = ExpandingStackedWidget()

        self.display_edits = {}
        self.name_edits = {}
        self.color_space_combos = {}
        self.view_transform_combos = {}
        self.looks_edits = {}
        self.rule_combos = {}
        self.description_edits = {}

        self.edit_shared_view_button = QtWidgets.QPushButton(
            self.model.get_view_type_icon(ViewType.VIEW_SHARED), "Edit Shared View"
        )

        for view_type in self.VIEW_LAYERS:
            params_layout = FormLayout()
            params_layout.setContentsMargins(0, 0, 0, 0)

            display_edit = LineEdit()
            self.display_edits[view_type] = display_edit
            params_layout.addRow(self.display_model.NAME.label, display_edit)

            if view_type in (ViewType.VIEW_DISPLAY, ViewType.VIEW_SCENE):
                name_edit = LineEdit()
                self.name_edits[view_type] = name_edit
                params_layout.addRow(self.model.NAME.label, name_edit)

                if view_type == ViewType.VIEW_DISPLAY:
                    view_transform_combo = CallbackComboBox(
                        ConfigCache.get_view_transform_names
                    )
                    self.view_transform_combos[view_type] = view_transform_combo
                    params_layout.addRow(
                        self.model.VIEW_TRANSFORM.label, view_transform_combo
                    )

                if view_type == ViewType.VIEW_SCENE:
                    get_view_color_space_names = (
                        lambda: ConfigCache.get_color_space_names(
                            ocio.SEARCH_REFERENCE_SPACE_SCENE
                        )
                    )
                else:  # ViewType.VIEW_DISPLAY
                    get_view_color_space_names = (
                        lambda: ConfigCache.get_color_space_names(
                            ocio.SEARCH_REFERENCE_SPACE_DISPLAY
                        )
                    )

                color_space_combo = CallbackComboBox(get_view_color_space_names)
                self.color_space_combos[view_type] = color_space_combo
                params_layout.addRow(self.model.COLOR_SPACE.label, color_space_combo)

                looks_edit = LineEdit()
                self.looks_edits[view_type] = looks_edit
                params_layout.addRow(self.model.LOOKS.label, looks_edit)

                if view_type == ViewType.VIEW_DISPLAY:
                    rule_combo = CallbackComboBox(ConfigCache.get_viewing_rule_names)
                    self.rule_combos[view_type] = rule_combo
                    params_layout.addRow(self.model.RULE.label, rule_combo)

                    description_edit = LineEdit()
                    self.description_edits[view_type] = description_edit
                    params_layout.addRow(self.model.DESCRIPTION.label, description_edit)

            elif view_type == ViewType.VIEW_SHARED:
                params_layout.addRow(self.edit_shared_view_button)

            params = QtWidgets.QFrame()
            params.setLayout(params_layout)
            self._param_stack.addWidget(params)

        self._param_layout.removeRow(0)
        self._param_layout.addRow(self._param_stack)

    def update_available_params(
        self,
        display_mapper: QtWidgets.QDataWidgetMapper,
        view_mapper: QtWidgets.QDataWidgetMapper,
        view_type: Optional[ViewType] = None,
    ) -> None:
        """
        Enable the interface needed to edit the current display and
        view.
        """
        display_mapper.clearMapping()
        view_mapper.clearMapping()

        # Track view mapper connections
        if view_mapper not in self._connected:
            self._connected[view_mapper] = []

        self._param_stack.setCurrentIndex(self.VIEW_LAYERS.index(view_type))

        display_mapper.addMapping(
            self.display_edits[view_type], self.display_model.NAME.column
        )

        if view_type in (ViewType.VIEW_DISPLAY, ViewType.VIEW_SCENE):
            view_mapper.addMapping(self.name_edits[view_type], self.model.NAME.column)

            color_space_combo = self.color_space_combos[view_type]
            view_mapper.addMapping(color_space_combo, self.model.COLOR_SPACE.column)

            # Trigger color space update before losing focus
            if color_space_combo not in self._connected[view_mapper]:
                color_space_combo.currentIndexChanged.connect(
                    partial(self.submit_mapper_deferred, view_mapper)
                )
                self._connected[view_mapper].append(color_space_combo)

            view_mapper.addMapping(self.looks_edits[view_type], self.model.LOOKS.column)

            if view_type == ViewType.VIEW_DISPLAY:
                view_transform_combo = self.view_transform_combos[view_type]
                view_mapper.addMapping(
                    view_transform_combo,
                    self.model.VIEW_TRANSFORM.column,
                )

                # Trigger view transform update before losing focus
                if view_transform_combo not in self._connected[view_mapper]:
                    view_transform_combo.currentIndexChanged.connect(
                        partial(self.submit_mapper_deferred, view_mapper)
                    )
                    self._connected[view_mapper].append(view_transform_combo)

                view_mapper.addMapping(
                    self.rule_combos[view_type], self.model.RULE.column
                )
                view_mapper.addMapping(
                    self.description_edits[view_type], self.model.DESCRIPTION.column
                )


class ViewEdit(BaseConfigItemEdit):
    """
    Widget for editing all displays and views in the current config.
    """

    shared_view_selection_requested = QtCore.Signal(str)

    __param_edit_type__ = ViewParamEdit

    @classmethod
    def item_type_label(cls, plural: bool = False) -> str:
        return f"Display{'s' if plural else ''} and View{'s' if plural else ''}"

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Try to preserve view choice through display changes
        self._prev_view = None

        self.display_model = self.param_edit.display_model

        # Widgets
        self.display_list = ItemModelListWidget(
            self.display_model,
            self.display_model.NAME.column,
            item_icon=DisplayModel.item_type_icon(),
        )
        self.display_list.current_row_changed.connect(self._on_display_changed)
        self.display_model.item_added.connect(self.display_list.set_current_item)
        self.display_model.item_selection_requested.connect(
            lambda index: self.display_list.set_current_row(index.row())
        )
        self.display_model.item_renamed.connect(self._on_display_renamed)

        # Map display widget to model
        self._display_mapper = QtWidgets.QDataWidgetMapper()
        self._display_mapper.setOrientation(QtCore.Qt.Horizontal)
        self._display_mapper.setSubmitPolicy(QtWidgets.QDataWidgetMapper.ManualSubmit)
        self._display_mapper.setModel(self.display_model)

        for view_type, display_edit in self.param_edit.display_edits.items():
            display_edit.editingFinished.connect(self._on_display_editing_finished)

        self.param_edit.edit_shared_view_button.released.connect(
            self._on_edit_shared_view_button_clicked
        )

        # Clear default mapped widgets from view model. Widgets will be remapped per
        # view type.
        self._mapper.clearMapping()

        # Layout
        self.splitter.insertWidget(0, self.display_list)

        # Initialize
        if self.display_model.rowCount():
            self.display_list.set_current_row(0)

    def _get_view_type(self, row: int) -> Optional[ViewType]:
        """
        :param row: Current view row
        :return: Current view type, if a view is selected
        """
        if row >= 0:
            return self.model.data(
                self.model.index(row, self.model.VIEW_TYPE.column),
                QtCore.Qt.EditRole,
            )
        else:
            # No view available or selected
            return None

    def _on_display_editing_finished(self) -> None:
        """
        Workaround for a Qt bug where the QLineEdit editingFinished
        signal is emitted twice when pressing enter. See:
            https://forum.qt.io/topic/39141/qlineedit-editingfinished-signal-is-emitted-twice
        """
        view_type = self._get_view_type(self.list.current_row())
        self.param_edit.display_edits[view_type].blockSignals(True)
        self._display_mapper.submit()
        self.param_edit.display_edits[view_type].blockSignals(False)

    @QtCore.Slot(str, str)
    def _on_display_renamed(self, display: str, prev_display: str) -> None:
        """
        Called when current display is renamed, to trigger subscription
        update for all views.

        :param display: New display name
        :param prev_display: Previous display name
        """
        for i in range(self.model.rowCount()):
            view_index = self.model.index(i, self.model.NAME.column)
            item_name = self.model.format_subscription_item_name(view_index)
            prev_item_name = self.model.format_subscription_item_name(
                view_index, display=prev_display
            )
            slot = TransformManager.get_subscription_slot(self.model, prev_item_name)
            if slot != -1:
                TransformManager.set_subscription(slot, self.model, item_name)

    @QtCore.Slot(int)
    def _on_display_changed(self, display_row: int) -> None:
        """
        Called when current display is changed, to trigger view list
        update.

        :param display_row: Current display row
        """
        self.param_edit.setEnabled(display_row >= 0)
        if display_row < 0:
            self.param_edit.reset()
        else:
            # Get display and view names
            display = self.display_model.data(
                self.display_model.index(display_row, self.display_model.NAME.column)
            )

            # Update view model
            self.model.set_display(display)

            # Update display mapper
            self._display_mapper.setCurrentIndex(display_row)

            # Update view list
            view_row = -1
            if self.model.rowCount():
                view_row = 0
                if self._prev_view is not None:
                    selected, other_view_row = self.list.set_current_item(
                        self._prev_view
                    )
                    if selected:
                        view_row = other_view_row

            self.list.set_current_row(view_row)

    def _on_edit_shared_view_button_clicked(self) -> None:
        """Goto and select the current shared view."""
        view_index = self.list.current_index()
        view = self.model.get_item_name(view_index)
        if view:
            self.shared_view_selection_requested.emit(view)

    @QtCore.Slot(int)
    def _on_current_row_changed(self, view_row: int) -> None:
        view_type = None
        if view_row != -1:
            self._prev_view = self.model.data(
                self.model.index(self.list.current_row(), self.model.NAME.column)
            )
            view_type = self.model.data(
                self.model.index(view_row, self.model.VIEW_TYPE.column),
                QtCore.Qt.EditRole,
            )

        # Update parameter widget states, since view type may have changed
        self.param_edit.update_available_params(
            self._display_mapper, self._mapper, view_type
        )

        # Update display params
        self._display_mapper.setCurrentIndex(self._display_mapper.currentIndex())

        # Update view params
        super()._on_current_row_changed(view_row)
