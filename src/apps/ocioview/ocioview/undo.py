# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import warnings
from types import TracebackType
from typing import Any, Optional

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui

from .config_cache import ConfigCache


undo_stack = QtGui.QUndoStack()
"""Global undo stack."""


class ItemModelUndoCommand(QtGui.QUndoCommand):
    """
    Undo command for use in item model ``setData`` implementations.

    .. note::
        These undo command implementations add themselves to the undo
        stack. Upon being added to the stack, their ``redo`` method
        will be called automatically.
    """

    def __init__(
        self,
        text: str,
        index: QtCore.QPersistentModelIndex,
        redo_value: Any,
        undo_value: Any,
        parent: Optional[QtGui.QUndoCommand] = None,
    ):
        """
        :param text: Undo/redo command menu text
        :param index: Persistent index of the modified item
        :param redo_value: Value to set initially on redo
        :param undo_value: Value to set on undo
        """
        super().__init__(text, parent=parent)

        self._index = index
        self._redo_value = redo_value
        self._undo_value = undo_value

        # Add self to undo stack
        undo_stack.push(self)

    def redo(self) -> None:
        if self._index.isValid():
            model = self._index.model()
            model.setData(self._index, self._redo_value)

    def undo(self) -> None:
        if self._index.isValid():
            model = self._index.model()
            model.setData(self._index, self._undo_value)


class ConfigSnapshotUndoCommand(QtGui.QUndoCommand):
    """
    Undo command for complex config changes like item adds, moves,
    and deletes, to be used as a content manager in which the entry
    state is the undo state and the exit state is the redo state.
    """

    def __init__(
        self,
        text: str,
        model: Optional[QtCore.QAbstractItemModel] = None,
        item_name: Optional[str] = None,
        parent: Optional[QtGui.QUndoCommand] = None,
    ):
        """
        :param text: Undo/redo command menu text
        :param model: Model to be reset on applied undo/redo
        :param item_name: Optional item name to try and select in the
            optionally provided model upon applying undo/redo.
        """
        super().__init__(text, parent=parent)

        self._model = model
        self._item_name = item_name
        self._undo_cache_id = None
        self._undo_state = None
        self._redo_cache_id = None
        self._redo_state = None

        # Since this undo command captures config changes within its managed context,
        # redo should not be called upon adding the command to the undo stack.
        self._init_command = True

    def __enter__(self) -> None:
        config = ocio.GetCurrentConfig()
        self._undo_cache_id, is_valid = ConfigCache.get_cache_id()
        if is_valid:
            self._undo_state = config.serialize()

    def __exit__(
        self, exc_type: type, exc_val: Exception, exc_tb: TracebackType
    ) -> None:
        config = ocio.GetCurrentConfig()
        self._redo_cache_id, is_valid = ConfigCache.get_cache_id()
        if is_valid:
            self._redo_state = config.serialize()

        # Add self to undo stack if config state could be captured
        if self._undo_state is not None and self._redo_state is not None:
            undo_stack.push(self)
            # Enable redo now that the command is in the stack
            self._init_command = False
        else:
            state_desc = []
            if self._undo_state is None:
                state_desc.append("starting")
            elif self._redo_state is None:
                state_desc.append("ending")

            warnings.warn(
                f"Command '{self.text()}' could not be added to the undo stack "
                f"because its {' and '.join(state_desc)} config "
                f"state{'s are' if len(state_desc) == 2 else ' is'} invalid."
            )

    def _apply_state(self, cache_id, state):
        """
        :param cache_id: Config cache ID to check. If this differs from
            the current config's cache ID, the provided config state
            will be restored.
        :param state: Serialized config data to restore
        """
        prev_item_names = []
        has_item_names = False
        new_cache_id = ConfigCache.get_cache_id()

        if new_cache_id != cache_id:
            if self._model is not None:
                self._model.beginResetModel()

                # Get current item names in the model
                if (
                    hasattr(self._model, "get_item_names")
                    and hasattr(self._model, "get_index_from_item_name")
                    and hasattr(self._model, "item_selection_requested")
                ):
                    prev_item_names = self._model.get_item_names()
                    has_item_names = True

            updated_config = ocio.Config.CreateFromStream(state)
            ocio.SetCurrentConfig(updated_config)

            if self._model is not None:
                self._model.endResetModel()

                if has_item_names:
                    next_item_names = self._model.get_item_names()

                    # Try to select a requested item name
                    if (
                        self._item_name is not None
                        and self._item_name in next_item_names
                    ):
                        index = self._model.get_index_from_item_name(self._item_name)
                        if index is not None:
                            self._model.item_selection_requested.emit(index)
                            return

                    # Try to select the first added item, if any
                    added_item_names = set(next_item_names).difference(prev_item_names)
                    if added_item_names:
                        for item_name in sorted(added_item_names):
                            index = self._model.get_index_from_item_name(item_name)
                            if index is not None:
                                self._model.item_selection_requested.emit(index)
                                return

                    # Try to select the first changed/reordered item, if any
                    elif (
                        len(next_item_names) == len(prev_item_names)
                        and next_item_names != prev_item_names
                    ):
                        for next_item_name, prev_item_name in zip(
                            next_item_names, prev_item_names
                        ):
                            if next_item_name != prev_item_name:
                                index = self._model.get_index_from_item_name(
                                    next_item_name
                                )
                                if index is not None:
                                    self._model.item_selection_requested.emit(index)
                                    return

                # Fallback to selecting first item
                column = 0
                if hasattr(self._model, "NAME"):
                    column = self._model.NAME.column
                self._model.item_selection_requested.emit(self._model.index(0, column))

    def redo(self) -> None:
        if not self._init_command:
            self._apply_state(self._redo_cache_id, self._redo_state)

    def undo(self) -> None:
        self._apply_state(self._undo_cache_id, self._undo_state)
