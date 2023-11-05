# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from functools import partial
from typing import Optional

from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import MARGIN_WIDTH
from ..transform_manager import TransformManager
from ..transforms import TransformEditStack
from ..utils import get_glyph_icon, SignalsBlocked
from ..widgets import FormLayout, LineEdit, ItemModelListWidget
from .config_item_model import ColumnDesc, BaseConfigItemModel
from .utils import adapt_splitter_sizes


class BaseConfigItemParamEdit(QtWidgets.QWidget):
    """
    Widget for editing the parameters and transforms for one config
    item model row.
    """

    # Config item model
    __model_type__: type = None

    # Whether the config item has an associated pair of transform stacks
    __has_transforms__: bool = False

    # Whether to wrap parameters in a tab. This is always True if __has_transforms__
    # is True.
    __has_tabs__: bool = False

    # Column descriptor for the "from_reference" equivalent transform model column
    __from_ref_column_desc__: ColumnDesc = None

    # Column descriptor for the "to_reference" equivalent transform model column
    __to_ref_column_desc__: ColumnDesc = None

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        self.model = self.__model_type__()
        self.model.modelAboutToBeReset.connect(self.reset)

        palette = self.palette()

        if self.__has_transforms__:
            self.__has_tabs__ = True
            no_tf_color = palette.color(palette.ColorGroup.Disabled, palette.ColorRole.Text)
            self._from_ref_icon = get_glyph_icon("mdi6.layers-plus")
            self._no_from_ref_icon = get_glyph_icon(
                "mdi6.layers-plus", color=no_tf_color
            )
            self._to_ref_icon = get_glyph_icon("mdi6.layers-minus")
            self._no_to_ref_icon = get_glyph_icon(
                "mdi6.layers-minus", color=no_tf_color
            )

        # Widgets
        self.name_edit = LineEdit()

        if self.__has_transforms__:
            self.from_ref_stack = TransformEditStack()
            self.from_ref_stack.edited.connect(
                partial(self._on_transform_edited, self.from_ref_stack)
            )
            self.to_ref_stack = TransformEditStack()
            self.to_ref_stack.edited.connect(
                partial(self._on_transform_edited, self.to_ref_stack)
            )

        # Layout
        self._param_layout = FormLayout()
        self._param_layout.addRow(self.model.NAME.label, self.name_edit)

        param_spacer_layout = QtWidgets.QVBoxLayout()
        param_spacer_layout.addLayout(self._param_layout)
        param_spacer_layout.addStretch()

        param_frame = QtWidgets.QFrame()
        param_frame.setLayout(param_spacer_layout)

        param_scroll_area = QtWidgets.QScrollArea()
        param_scroll_area.setObjectName("config_item_param_edit__param_scroll_area")
        param_scroll_area.setSizePolicy(
            QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding
        )
        param_scroll_area.setWidgetResizable(True)
        if self.__has_tabs__:
            param_scroll_area.setStyleSheet(
                f"QScrollArea#config_item_param_edit__param_scroll_area {{"
                f"    border: none;"
                f"    border-top: 1px solid "
                f"        {palette.color(QtGui.QPalette.Dark).name()};"
                f"    margin-top: {MARGIN_WIDTH:d}px;"
                f"}}"
            )
        param_scroll_area.setWidget(param_frame)

        if self.__has_tabs__:
            self.tabs = QtWidgets.QTabWidget()
            self.tabs.addTab(
                param_scroll_area,
                self.model.item_type_icon(),
                self.model.item_type_label(),
            )
            if self.__has_transforms__:
                self.tabs.addTab(
                    self.from_ref_stack,
                    self._no_from_ref_icon,
                    self.__from_ref_column_desc__.label,
                )
                self.tabs.addTab(
                    self.to_ref_stack,
                    self._no_to_ref_icon,
                    self.__to_ref_column_desc__.label,
                )

        layout = QtWidgets.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        if self.__has_tabs__:
            layout.addWidget(self.tabs)
        else:
            layout.addWidget(param_scroll_area)
        self.setLayout(layout)

    def reset(self) -> None:
        """
        Reset all parameter widgets, for cases where there are no rows
        in model.
        """
        if self.__has_transforms__:
            self.from_ref_stack.reset()
            self.to_ref_stack.reset()

        for i in range(self._param_layout.count()):
            item = self._param_layout.itemAt(i)
            if item is not None:
                widget = item.widget()
                if widget is not None:
                    if hasattr(widget, "reset"):
                        widget.reset()

    def submit_mapper_deferred(
        self, mapper: QtWidgets.QDataWidgetMapper, *args, **kwargs
    ):
        """
        Call 'submit' on a data widget mapper after a brief delay. This
        allows a modified widget to be repainted before updating the
        current config, which may have expensive side effects.
        """
        QtCore.QTimer.singleShot(10, mapper.submit)

    def update_transform_tab_icons(self) -> None:
        """
        Update transform tab icons, indicating which of the tabs
        contain transforms.
        """
        for tf_edit_stack in (self.from_ref_stack, self.to_ref_stack):
            self._on_transform_edited(tf_edit_stack)

    def _on_transform_edited(self, tf_edit_stack: TransformEditStack) -> None:
        """
        :param tf_edit_stack: Transform stack containing the
            last edited transform edit widget.
        """
        if self.__has_transforms__:
            if tf_edit_stack.transform_count():
                self.tabs.setTabIcon(
                    self.tabs.indexOf(tf_edit_stack),
                    self._from_ref_icon
                    if tf_edit_stack == self.from_ref_stack
                    else self._to_ref_icon,
                )
            else:
                self.tabs.setTabIcon(
                    self.tabs.indexOf(tf_edit_stack),
                    self._no_from_ref_icon
                    if tf_edit_stack == self.from_ref_stack
                    else self._no_to_ref_icon,
                )


class BaseConfigItemEdit(QtWidgets.QWidget):
    """
    Widget for editing an item model for the current config.
    """

    # Corresponding BaseConfigItemParamEdit type
    __param_edit_type__: type = None

    # Whether there are many config items, which need to be managed through an
    # item list.
    __has_list__: bool = True

    # Whether the primary widgets of this item edit should be wrapped in a splitter.
    # By default, this inherits from __has_list__.
    __has_splitter__: bool = None

    @classmethod
    def item_type_icon(cls) -> QtGui.QIcon:
        """
        :return: Item type icon
        """
        return cls.__param_edit_type__.__model_type__.item_type_icon()

    @classmethod
    def item_type_label(cls, plural: bool = False) -> str:
        """
        :param plural: Whether label should be plural
        :return: Friendly type name
        """
        return cls.__param_edit_type__.__model_type__.item_type_label(plural=plural)

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent=parent)

        # Widgets
        self.param_edit = self.__param_edit_type__()
        if self.__has_list__:
            self.param_edit.setEnabled(False)

        model = self.model

        if self.__has_list__:
            self.list = ItemModelListWidget(
                model, model.NAME.column, item_icon=self.item_type_icon()
            )
            self.list.view.installEventFilter(self)
            self.list.current_row_changed.connect(self._on_current_row_changed)

            model.item_selection_requested.connect(
                lambda index: self.list.set_current_row(index.row())
            )
            model.modelAboutToBeReset.connect(lambda: self.param_edit.setEnabled(False))

        # Map widgets to model columns
        self._mapper = QtWidgets.QDataWidgetMapper()
        self._mapper.setOrientation(QtCore.Qt.Horizontal)
        self._mapper.setSubmitPolicy(QtWidgets.QDataWidgetMapper.AutoSubmit)
        self._mapper.setModel(model)

        try:
            self._mapper.addMapping(self.param_edit.name_edit, model.NAME.column)
        except RuntimeError:
            # Some derived classes may delete this widget to handle custom mapping
            pass

        # NOTE: Using the data widget mapper to set/get transforms results in consistent
        #       crashing, presumably due to transform references being garbage collected
        #       in transit. Conversely, handling transform updates manually via
        #       signals/slots is stable, so used here instead.
        if self.param_edit.__has_transforms__:
            self.param_edit.from_ref_stack.edited.connect(self._on_from_ref_edited)
            self.param_edit.to_ref_stack.edited.connect(self._on_to_ref_edited)
            model.dataChanged.connect(self._on_data_changed)

        # Layout
        if self.__has_splitter__ is None:
            self.__has_splitter__ = self.__has_list__

        if self.__has_splitter__:
            self.splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)
            self.splitter.setOpaqueResize(False)
            if self.__has_list__:
                self.splitter.addWidget(self.list)
            self.splitter.addWidget(self.param_edit)

        layout = QtWidgets.QVBoxLayout()
        if self.__has_splitter__:
            layout.addWidget(self.splitter)
        else:
            layout.addWidget(self.param_edit)
        self.setLayout(layout)

    @property
    def model(self) -> BaseConfigItemModel:
        return self.param_edit.model

    def set_splitter_sizes(self, from_sizes: list[int]) -> None:
        """
        Update splitter to match the provided sizes.

        :param from_sizes: Sizes to match, with emphasis on matching
            the first index.
        """
        if self.__has_splitter__:
            self.splitter.setSizes(
                adapt_splitter_sizes(from_sizes, self.splitter.sizes())
            )

    def eventFilter(self, watched: QtCore.QObject, event: QtCore.QEvent) -> bool:
        """
        Handle setting subscription for the current item's transform on
        number key press.
        """
        if (
            self.__has_list__
            and watched == self.list.view
            and event.type() == QtCore.QEvent.KeyRelease
            and event.key()
            in (
                QtCore.Qt.Key_0,
                QtCore.Qt.Key_1,
                QtCore.Qt.Key_2,
                QtCore.Qt.Key_3,
                QtCore.Qt.Key_4,
                QtCore.Qt.Key_5,
                QtCore.Qt.Key_6,
                QtCore.Qt.Key_7,
                QtCore.Qt.Key_8,
                QtCore.Qt.Key_9,
            )
        ):
            current_index = self.list.current_index()
            item_name = self.model.format_subscription_item_name(current_index)
            if item_name:
                TransformManager.set_subscription(
                    int(event.text()), self.model, item_name
                )
                return True

        return False

    @QtCore.Slot(int)
    def _on_current_row_changed(self, row: int) -> None:
        """
        Load the item defined in the model at the specified row.
        """
        self.param_edit.setEnabled(row >= 0)
        if row < 0:
            self.param_edit.reset()
        else:
            self._mapper.setCurrentIndex(row)

            # Manually update transform stacks from model, on current row change
            if self.param_edit.__has_transforms__:
                model = self.model

                with SignalsBlocked(
                    self.param_edit.from_ref_stack, self.param_edit.to_ref_stack
                ):
                    self.param_edit.from_ref_stack.set_transform(
                        model.data(
                            model.index(
                                row, self.param_edit.__from_ref_column_desc__.column
                            ),
                            QtCore.Qt.EditRole,
                        )
                    )
                    self.param_edit.to_ref_stack.set_transform(
                        model.data(
                            model.index(
                                row, self.param_edit.__to_ref_column_desc__.column
                            ),
                            QtCore.Qt.EditRole,
                        )
                    )
                self.param_edit.update_transform_tab_icons()

    @QtCore.Slot(QtCore.QModelIndex, QtCore.QModelIndex, list)
    def _on_data_changed(
        self, top_left: QtCore.QModelIndex, bottom_right: QtCore.QModelIndex, roles=()
    ) -> None:
        """
        Manually update transform stacks from model, on model data
        change.
        """
        if QtCore.Qt.EditRole not in roles:
            return

        if self.param_edit.__has_transforms__:
            column = top_left.column()
            if column == self.param_edit.__from_ref_column_desc__.column:
                with SignalsBlocked(self.param_edit.from_ref_stack):
                    self.param_edit.from_ref_stack.set_transform(
                        self.model.data(top_left, QtCore.Qt.EditRole)
                    )
            elif column == self.param_edit.__to_ref_column_desc__.column:
                with SignalsBlocked(self.param_edit.to_ref_stack):
                    self.param_edit.to_ref_stack.set_transform(
                        self.model.data(top_left, QtCore.Qt.EditRole)
                    )

    def _on_from_ref_edited(self) -> None:
        """
        Manually update model from transform stack, on transform
        change.
        """
        if self.param_edit.__has_transforms__ and self.__has_list__:
            model = self.model

            current_index = self.list.current_index()
            if current_index is not None:
                model.setData(
                    model.index(
                        current_index.row(),
                        self.param_edit.__from_ref_column_desc__.column,
                    ),
                    self.param_edit.from_ref_stack.transform(),
                )

    def _on_to_ref_edited(self) -> None:
        """
        Manually update model from transform stack, on transform
        change.
        """
        if self.param_edit.__has_transforms__ and self.__has_list__:
            model = self.model

            current_index = self.list.current_index()
            if current_index is not None:
                model.setData(
                    model.index(
                        current_index.row(),
                        self.param_edit.__to_ref_column_desc__.column,
                    ),
                    self.param_edit.to_ref_stack.transform(),
                )
