# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import logging
from dataclasses import dataclass
from typing import Any, Optional, Type, Union

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from ..config_cache import ConfigCache
from ..transform_manager import TransformManager, TransformAgent
from ..undo import ItemModelUndoCommand, ConfigSnapshotUndoCommand
from ..utils import get_glyph_icon, next_name, item_type_label


@dataclass
class ColumnDesc:
    """
    Dataclass which describes an item model column with its label and
    data type.
    """

    column: int
    label: str
    type: type


class BaseConfigItemModel(QtCore.QAbstractTableModel):
    """
    Abstract item model class for editing one type of OCIO config
    object (ColorSpace, Look, etc.) in the current config.
    """

    item_renamed = QtCore.Signal(str, str)
    item_added = QtCore.Signal(str)
    item_moved = QtCore.Signal()
    item_removed = QtCore.Signal()
    item_selection_requested = QtCore.Signal(QtCore.QModelIndex)
    warning_raised = QtCore.Signal(str)

    # Agents broadcast transform and name changes to subscribers
    _tf_agents = [TransformAgent(i) for i in range(10)]

    # Implementations must include a name column, and define all other
    # implemented column description constants here.
    NAME = ColumnDesc(0, "Name", str)

    COLUMNS = [NAME]
    """Ordered model columns."""

    NULL_INDEX = QtCore.QModelIndex()
    """
    Default constructed (null) model index, indicating an invalid or 
    unused index.
    """

    # OCIO config object type this model manages.
    __item_type__: type = None

    # OCIO config object type label for use in GUI components, generated on first call
    # to ``item_type_label()``.
    __item_type_label__: str = None

    # QtAwesome glyph name to use for this item type's icon
    __icon_glyph__: str = None

    # Item type icon, loaded on first call to ``transform_type_icon()``.
    __icon__: QtGui.QIcon = None

    @classmethod
    def item_type_icon(cls) -> QtGui.QIcon:
        """
        :return: Item type icon
        """
        if cls.__icon__ is None:
            cls.__icon__ = get_glyph_icon(cls.__icon_glyph__)
        return cls.__icon__

    @classmethod
    def item_type_label(cls, plural: bool = False) -> str:
        """
        :param plural: Whether label should be plural
        :return: Friendly type name
        """
        if cls.__item_type_label__ is None:
            cls.__item_type_label__ = item_type_label(cls.__item_type__)
        if plural:
            return cls.__item_type_label__ + "s"
        else:
            return cls.__item_type_label__

    @classmethod
    def get_transform_agent(cls, slot: int) -> Optional[TransformAgent]:
        """
        Get the transform subscription agent for the specified slot.

        :param slot: Subscription slot number between 1-10
        :return: Transform subscription agent, or None for an invalid
            slot number.
        """
        if 0 <= slot < 10:
            return cls._tf_agents[slot]
        else:
            return None

    @classmethod
    def has_presets(cls) -> bool:
        """
        Subclasses must indicate whether the model supports adding
        preset items.

        :return: Whether presets are supported
        """
        return False

    @classmethod
    def requires_presets(cls) -> bool:
        """
        Subclasses must indicate whether presets are required (only
        presets can be added to model).

        :return: Whether presets are required
        """
        return False

    @classmethod
    def get_presets(cls) -> Optional[Union[list[str], dict[str, QtGui.QIcon]]]:
        """
        Subclasses may return preset items to make available in a view.

        :return: list of preset names or dictionary of preset names and
            icons.
        """
        return None

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Prefix for new items without provided names
        self._item_prefix = f"{self.__item_type__.__name__}_"

        # Temporary item storage for when rebuilding config section
        self._items = None

    def reset(self) -> None:
        """Reset all model data."""
        self.beginResetModel()
        self._reset_cache()
        self.endResetModel()

        # Request selection of first item
        item_names = self.get_item_names()
        if item_names:
            first_item_index = self.get_index_from_item_name(item_names[0])
            if first_item_index != self.NULL_INDEX:
                self.item_selection_requested.emit(first_item_index)

    def repaint(self) -> None:
        """Force all items to be repainted in all views."""
        self.dataChanged.emit(
            self.index(0, self.NAME.column),
            self.index(self.rowCount() - 1, self.NAME.column),
        )

    def add_preset(self, preset_name: str) -> int:
        """
        Subclasses may implement preset item behavior in this method.
        By default, it does nothing.

        :param preset_name: Name of preset to add
        :return: Added item row
        """
        return -1

    def create_item(self, name: Optional[str] = None) -> int:
        """
        Create a new item and add it to the current config, generating
        a unique name if none is provided.

        :param name: Optional item name
        :return: Item row
        """
        row = -1

        if not name:
            item_names = self.get_item_names() + ConfigCache.get_all_names()
            name = next_name(self._item_prefix, item_names)

        with ConfigSnapshotUndoCommand(
            f"Create {self.item_type_label()}", model=self, item_name=name
        ):
            self._new_item(name)
            index = self.get_index_from_item_name(name)

            # Was an item created?
            if index is not None:
                row = index.row()

                self.beginInsertRows(self.NULL_INDEX, row, row)
                self.endInsertRows()
                self.item_added.emit(name)

        return row

    def move_item_up(self, item_name: str) -> bool:
        """
        Move the named item up one row, if possible.

        :param item_name: Name of item to move
        :return: Whether the item was moved
        """
        item_names = self.get_item_names()
        if item_name not in item_names:
            return False

        src_row = item_names.index(item_name)
        dst_row = max(0, src_row - 1)

        if dst_row == src_row:
            return False

        return self.moveRows(self.NULL_INDEX, src_row, 1, self.NULL_INDEX, dst_row)

    def move_item_down(self, item_name: str) -> bool:
        """
        Move the named item down one row, if possible.

        :param item_name: Name of item to move
        :return: Whether the item was moved
        """
        item_names = self.get_item_names()
        if item_name not in item_names:
            return False

        src_row = item_names.index(item_name)
        dst_row = min(len(item_names) - 1, src_row + 1)

        if dst_row == src_row:
            return False

        return self.moveRows(self.NULL_INDEX, src_row, 1, self.NULL_INDEX, dst_row)

    def flags(self, index: QtCore.QModelIndex) -> int:
        return QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled

    def rowCount(self, parent: QtCore.QModelIndex = NULL_INDEX) -> int:
        """
        :return: Number of items defined in the current config
        """
        return len(self._get_items())

    def columnCount(self, parent: QtCore.QModelIndex = NULL_INDEX) -> int:
        return len(self.COLUMNS)

    def headerData(
        self,
        column: int,
        orientation: QtCore.Qt.Orientation,
        role: int = QtCore.Qt.DisplayRole,
    ) -> Any:
        """
        :return: Column labels
        """
        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            return self.COLUMNS[column].label

    def data(self, index: QtCore.QModelIndex, role: int = QtCore.Qt.DisplayRole) -> Any:
        """
        :return: Item data pulled from the current config for the
            index-referenced column.
        """
        item, column_desc = self._get_item_and_column(index)
        if item is None:
            return None

        if role == QtCore.Qt.DisplayRole:
            return self._get_display(item, column_desc)
        elif role == QtCore.Qt.EditRole:
            return self._get_value(item, column_desc)
        elif role == QtCore.Qt.DecorationRole:
            return self._get_icon(item, column_desc)
        elif role == QtCore.Qt.BackgroundRole:
            return self._get_bg_color(item, column_desc)
        elif role == QtCore.Qt.CheckStateRole:
            # Get check state from configured column
            checked_column_desc = self._get_checked_column()
            if checked_column_desc is not None:
                checked = self._get_value(item, checked_column_desc)
                return QtCore.Qt.Checked if checked else QtCore.Qt.Unchecked

        return None

    def setData(
        self, index: QtCore.QModelIndex, value: Any, role: int = QtCore.Qt.EditRole
    ) -> bool:
        """
        Push modified item data to the current config for the
        index-referenced column.
        """
        if role not in (QtCore.Qt.EditRole, QtCore.Qt.CheckStateRole):
            return False

        item, column_desc = self._get_item_and_column(index)
        if item is None:
            return False

        if role == QtCore.Qt.CheckStateRole:
            # Route check state to configured column
            checked_column_desc = self._get_checked_column()
            if checked_column_desc is not None:
                column_desc = checked_column_desc
                index = index.sibling(index.row(), column_desc.column)
                value = value == QtCore.Qt.Checked
                role = QtCore.Qt.EditRole

        undo_cmd_type = self._get_undo_command_type(column_desc)

        current_value = self._get_value(item, column_desc)
        data_changed = current_value != value

        if data_changed:
            if not isinstance(index, QtCore.QPersistentModelIndex):
                if undo_cmd_type == ItemModelUndoCommand:
                    # Add undo command to undo stack on initial invocation, which will
                    # cycle back through this method by calling 'redo' with the new
                    # persistent index.
                    ItemModelUndoCommand(
                        self._get_undo_command_text(index, column_desc),
                        QtCore.QPersistentModelIndex(index),
                        value,
                        current_value,
                    )
                    return False

            if undo_cmd_type == ConfigSnapshotUndoCommand:
                # Capture immediate change and side effects from the dataChanged
                # signal with undo command.
                with ConfigSnapshotUndoCommand(
                    self._get_undo_command_text(index, column_desc),
                    model=self,
                    item_name=self.get_item_name(index),
                ):
                    self._set_value(item, column_desc, value, index)
                    self.dataChanged.emit(index, index, [role])
            else:
                self._set_value(item, column_desc, value, index)
                self.dataChanged.emit(index, index, [role])

        return data_changed

    def insertRows(
        self, row: int, count: int, parent: QtCore.QModelIndex = NULL_INDEX
    ) -> bool:
        """
        Create ``count`` new items and add them to the current
        config at ``row`` index.

        Due to new config items generally being appended to the
        relevant config section, all similar items are first removed
        from the config (after being preserved in memory), and then
        re-added with the new items inserted at the requested index.
        """
        self.beginInsertRows(parent, row, row + count - 1)

        preserved_items = self._get_items(preserve=True)
        item_names = self.get_item_names() + ConfigCache.get_all_names()

        new_names = []
        for _ in range(count):
            name = next_name(self._item_prefix, item_names + new_names)
            new_names.append(name)

        with ConfigSnapshotUndoCommand(
            f"Add {self.item_type_label()}", model=self, item_name=new_names[0]
        ):
            self._clear_items()

            if preserved_items:
                for other_row, item in enumerate(preserved_items):
                    if other_row == row:
                        for name in new_names:
                            self._new_item(name)
                    self._add_item(item)
            else:
                for name in new_names:
                    self._new_item(name)

            self.endInsertRows()

            for name in new_names:
                self.item_added.emit(name)

        return True

    def moveRows(
        self,
        src_parent: QtCore.QModelIndex,
        src_row: int,
        count: int,
        dst_parent: QtCore.QModelIndex,
        dst_row: int,
    ) -> bool:
        """
        Move ``count`` items to the specified destination index in the
        current config.

        Due to new config items generally being appended to the
        relevant config section, all items are first removed from the
        config (after being preserved in memory), and then re-added
        in new order.
        """
        dst_row += 1 if dst_row > src_row else 0

        if self.beginMoveRows(
            src_parent, src_row, src_row + count - 1, dst_parent, dst_row
        ):
            all_names = self.get_item_names()
            all_items = {
                name: item
                for name, item in zip(all_names, self._get_items(preserve=True))
            }

            insert_before = None
            if dst_row < len(all_items):
                insert_before = all_names[dst_row]

            move_names = [
                all_names.pop(i) for i in reversed(range(src_row, src_row + count))
            ]

            if insert_before is not None:
                new_dst_row = all_names.index(insert_before)
            else:
                new_dst_row = len(all_names)

            with ConfigSnapshotUndoCommand(
                f"Move {self.item_type_label()}", model=self, item_name=move_names[0]
            ):
                for i in range(len(move_names)):
                    all_names.insert(new_dst_row, move_names[i])

                    self._clear_items()
                    for name in all_names:
                        self._add_item(all_items[name])

                    self.endMoveRows()
                    self.item_moved.emit()

            return True

        return False

    def removeRows(
        self, row: int, count: int, parent: QtCore.QModelIndex = NULL_INDEX
    ) -> bool:
        """
        Remove ``count`` items from the current config, starting at
        ``row`` index.
        """
        self.beginRemoveRows(parent, row, row + count - 1)

        items = self._get_items()
        item_names = self.get_item_names()
        num_items = len(items)
        do_not_remove = []

        with ConfigSnapshotUndoCommand(
            f"Delete {self.item_type_label()}", model=self, item_name=item_names[row]
        ):
            for i in reversed(range(row, row + count)):
                if i < num_items:
                    item = items[i]
                    can_be_removed, reason = self._can_item_be_removed(item)
                    if not can_be_removed:
                        do_not_remove.append((item_names[i], reason))
                    else:
                        self._remove_item(item)

            if num_items:
                self.item_removed.emit()

            self.endRemoveRows()

        # Warn user about refused item removals
        if do_not_remove:
            item_warning_lines = []
            for item_name, reason in do_not_remove:
                item_warning_lines.append(f"<b>{item_name}</b> {reason}")
            item_warnings = "<br><br>".join(item_warning_lines)

            self.warning_raised.emit(
                f"{len(do_not_remove)} "
                f"{self.item_type_label(plural=len(do_not_remove) != 1).lower()} could "
                f"not be removed:<br><br>{item_warnings}"
            )

        return True

    def get_item_names(self) -> list[str]:
        """
        :return: All item names
        """
        raise NotImplementedError

    def get_item_name(self, index: QtCore.QModelIndex) -> Optional[str]:
        """
        :param index: Model index for item
        :return: Item name, if available
        """
        return self.data(self.index(index.row(), self.NAME.column))

    def format_subscription_item_name(
        self, item_name_or_index: Union[str, QtCore.QModelIndex], **kwargs
    ) -> Optional[str]:
        """
        Format item name into a per-model unique name for tracking
        transform subscriptions.

        :param item_name_or_index: Item name or model index for item
        :return: Subscription unique item name
        """
        if isinstance(item_name_or_index, QtCore.QModelIndex):
            item_name = self.get_item_name(item_name_or_index)
        else:
            item_name = item_name_or_index
        return f"{item_name} [{self.item_type_label().lower()}]"

    def extract_subscription_item_name(self, subscription_item_name: str) -> str:
        """
        Unformat item name from its per-model unique name for tracking
        transform subscriptions.

        :param subscription_item_name: Subscription unique item name
        :return: Extracted item name
        """
        suffix = f" [{self.item_type_label().lower()}]"
        if subscription_item_name.endswith(suffix):
            return subscription_item_name[: -len(suffix)]
        else:
            return subscription_item_name

    def get_item_transforms(
        self, item_name: str
    ) -> tuple[Optional[ocio.Transform], Optional[ocio.Transform]]:
        """
        :param item_name: Item name to get transform for
        :return: Forward and inverse item transforms, or None if either
            is not defined.
        """
        return None, None

    def get_index_from_item_name(self, item_name: str) -> Optional[QtCore.QModelIndex]:
        """
        Lookup the model index for the named item.

        :param item_name: Item name to lookup
        :return: Item model index, if found
        """
        indexes = self.match(
            self.index(0, self.NAME.column),
            QtCore.Qt.DisplayRole,
            item_name,
            1,
            QtCore.Qt.MatchExactly | QtCore.Qt.MatchWrap,
        )
        if indexes:
            return indexes[0]
        else:
            return None

    def _reset_cache(self) -> None:
        """Reset internal config cache."""
        self._items = None

    def _get_item_and_column(
        self, index: QtCore.QModelIndex
    ) -> tuple[Optional[__item_type__], Optional[ColumnDesc]]:
        items = self._get_items()
        if items:
            try:
                return items[index.row()], self.COLUMNS[index.column()]
            except IndexError:
                # Item may have been removed
                logging.warning(f"{self} index {index} is invalid")
        return None, None

    def _get_undo_command_type(
        self, column_desc: ColumnDesc
    ) -> Type[QtGui.QUndoCommand]:
        """
        Support overriding the undo command type used to
        track data changes, per column.

        :param column_desc: Description of column being modified
        :return: Undo command type to track change
        """
        return ItemModelUndoCommand

    def _get_undo_command_text(
        self, index: QtCore.QModelIndex, column_desc: ColumnDesc
    ) -> str:
        """
        Format undo/redo action text from the item associated with the
        provided index and column.

        :param index: Model index being modified
        :param column_desc: Description of column being modified
        :return: Undo command text
        """
        # Get item name for undo/redo action text
        item_name = self.get_item_name(index)
        if item_name:
            item_desc = f" ({item_name})"
        else:
            item_desc = ""

        return f"Edit {self.item_type_label()} {column_desc.label}{item_desc}"

    def _can_item_be_removed(self, item: __item_type__) -> tuple[bool, str]:
        """
        Subclasses can override this method to prevent an item from
        being deleted by a user. Items should not be deleted when they
        are referenced elsewhere in the config.

        :param item: Item to check
        :return: Whether the item can safely be deleted, and a
            descriptive message with a reason if it can't.
        """
        return True, ""

    def _get_display(self, item: __item_type__, column_desc: ColumnDesc) -> str:
        """
        :return: Display role value for a given model column
        """
        value = self._get_value(item, column_desc)

        if column_desc.type == ocio.Transform:
            return value.__class__.__name__
        elif column_desc.type == list:
            return ", ".join(map(str, value))
        elif hasattr(column_desc.type, "__members__"):
            return value.name
        else:
            return str(value)

    def _get_bg_color(
        self, item: __item_type__, column_desc: ColumnDesc
    ) -> Optional[QtGui.QColor]:
        """
        :return: Background role value for a given model column
        """
        return None

    def _get_subscription_color(
        self, item: __item_type__, column_desc: ColumnDesc
    ) -> Optional[QtGui.QColor]:
        """
        If the item is set as a transform subscription, return a
        color for its subscription slot.
        """
        slot = TransformManager.get_subscription_slot(
            self,
            self.format_subscription_item_name(self._get_value(item, column_desc)),
        )
        return TransformManager.get_subscription_slot_color(
            slot, saturation=0.25, value=0.25
        )

    def _get_subscription_icon(
        self, item: __item_type__, column_desc: ColumnDesc
    ) -> Optional[QtGui.QIcon]:
        """
        If the item is set as a transform subscription, return a number
        icon for its subscription slot.
        """
        slot = TransformManager.get_subscription_slot(
            self,
            self.format_subscription_item_name(self._get_value(item, column_desc)),
        )
        return TransformManager.get_subscription_slot_icon(slot)

    def _get_icon(
        self, item: __item_type__, column_desc: ColumnDesc
    ) -> Optional[QtGui.QIcon]:
        """
        :return: Icon for a given model column
        """
        if column_desc == self.NAME:
            return self.item_type_icon()
        else:
            return None

    def _get_items(self, preserve: bool = False) -> list[__item_type__]:
        """
        :param preserve: Whether to preserve the config items in
            local memory prior to returning them. All items should
            be able to be removed from the current config without
            incurring data loss when re-adding them later.
        :return: All items of the configured type from the current
            config.
        """
        raise NotImplementedError

    def _clear_items(self) -> None:
        """
        Remove all items of the configured type from the current
        config.
        """
        raise NotImplementedError

    def _add_item(self, item: __item_type__) -> None:
        """
        Add the provided item to the current config.
        """
        raise NotImplementedError

    def _remove_item(self, item: __item_type__) -> None:
        """
        Remove the provided item from the current config.
        """
        raise NotImplementedError

    def _new_item(self, name: str) -> None:
        """
        Create a new item with the specified name and add it to the
        current config.
        """
        raise NotImplementedError

    def _get_checked_column(self) -> Optional[ColumnDesc]:
        """
        :return: Column description for the column that holds whether
            an item is checked (which can differ from the column that
            displays a checkbox). Return `None` if items have no check
            state (the default).
        """
        return None

    def _update_tf_subscribers(
        self, item_name: str, prev_item_name: Optional[str] = None
    ) -> None:
        """
        Broadcast transform and/or name changes to item transform
        subscribers.

        :param item_name: Name of changed item
        :param prev_item_name: Optional previous item name, if the
            name changed.
        """
        # Name adjustment may be needed for unique item/transform identifiers within
        # the model.
        item_name = self.format_subscription_item_name(item_name)
        if prev_item_name:
            prev_item_name = self.format_subscription_item_name(prev_item_name)

        # Is item set as a subscription?
        slot = TransformManager.get_subscription_slot(self, prev_item_name or item_name)
        if slot != -1:
            # Broadcast name change
            if prev_item_name and prev_item_name != item_name:
                self._tf_agents[slot].item_name_changed.emit(item_name)

            # Broadcast transform change
            self._tf_agents[slot].item_tf_changed.emit(
                *self.get_item_transforms(item_name)
            )

    def _get_value(self, item: __item_type__, column_desc: ColumnDesc) -> Any:
        """
        :return: Item parameter value referred to by the requested
            model column. This pulls data from the current config.
        """
        raise NotImplementedError

    def _set_value(
        self,
        item: __item_type__,
        column_desc: ColumnDesc,
        value: Any,
        index: QtCore.QModelIndex,
    ) -> None:
        """
        Set the item parameter value referred to by the specified model
        column. This pushes data to the current config.

        :return: A set of roles that should be updated for this model
            item in all views.
        """
        raise NotImplementedError
