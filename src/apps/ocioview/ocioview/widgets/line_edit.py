# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from contextlib import contextmanager
from pathlib import Path
from typing import Any, Optional, Sequence

from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import R_COLOR, G_COLOR, B_COLOR
from ..utils import SignalsBlocked, get_glyph_icon


class LineEdit(QtWidgets.QLineEdit):
    def __init__(
        self, text: Optional[str] = None, parent: Optional[QtCore.QObject] = None
    ):
        super().__init__(parent=parent)

        if text is not None:
            self.setText(str(text))

    # DataWidgetMapper user property interface
    @QtCore.Property(str, user=True)
    def __data(self) -> str:
        return self.value()

    @__data.setter
    def __data(self, data: str) -> None:
        with SignalsBlocked(self):
            self.set_value(data)

    # Common public interface
    def value(self) -> str:
        return self.text()

    def set_value(self, value: str) -> None:
        self.setText(str(value))

    def reset(self) -> None:
        self.clear()


class PathEdit(LineEdit):
    """
    File or directory path line edit with browse button.
    """

    BROWSE_GLYPHS = {
        QtWidgets.QFileDialog.AnyFile: "ph.file",
        QtWidgets.QFileDialog.ExistingFile: "ph.file",
        QtWidgets.QFileDialog.Directory: "ph.folder",
        QtWidgets.QFileDialog.ShowDirsOnly: "ph.folder",
    }

    def __init__(
        self,
        file_mode: QtWidgets.QFileDialog.FileMode,
        path: Optional[Path] = None,
        browse_filter: str = "",
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param file_mode: Defines the type of filesystem item to browse
            for.
        :param path: Optional initial path
        :param browse_filter: Optional file browser filter (see
            QFileBrowser documentation for details).
        """
        super().__init__(parent=parent)

        self._file_mode = file_mode
        self._filter = browse_filter

        if self._file_mode in self.BROWSE_GLYPHS:
            self._browse_action = self.addAction(
                get_glyph_icon(self.BROWSE_GLYPHS[self._file_mode]),
                self.ActionPosition.TrailingPosition,
            )
            self._browse_action.triggered.connect(self._on_browse_action_triggered)

        if path is not None:
            self.set_path(path)

    # DataWidgetMapper user property interface
    @QtCore.Property(Path, user=True)
    def __data(self) -> Path:
        return self.path()

    @__data.setter
    def __data(self, data: Path) -> None:
        with SignalsBlocked(self):
            self.set_path(data)

    # Direct path access
    def path(self) -> Path:
        return Path(self.text())

    def set_path(self, path: Path) -> None:
        self.setText(path.as_posix())

    def _on_browse_action_triggered(self) -> None:
        """
        Browse file system for file or directory.
        """
        # Get directory path from current text
        path = self.path()
        if path.is_file() or path.suffix:
            path = path.parent
        dir_str = path.as_posix()

        # Configure browser
        kwargs = {"dir": dir_str}
        if self._file_mode in (
            QtWidgets.QFileDialog.AnyFile,
            QtWidgets.QFileDialog.ExistingFile,
        ):
            kwargs["filter"] = self._filter
        elif self._file_mode == QtWidgets.QFileDialog.DirectoryOnly:
            kwargs["options"] = QtWidgets.QFileDialog.ShowDirsOnly

        # Browse...
        if self._file_mode == QtWidgets.QFileDialog.AnyFile:
            path_str = QtWidgets.QFileDialog.getSaveFileName(
                self, caption="Save As File", **kwargs
            )
        elif self._file_mode == QtWidgets.QFileDialog.ExistingFile:
            path_str = QtWidgets.QFileDialog.getOpenFileName(
                self, caption="Choose File", **kwargs
            )
        else:
            path_str = QtWidgets.QFileDialog.getExistingDirectory(
                self, caption="Choose Directory", **kwargs
            )

        # Push path back to line edit
        if path_str:
            self.set_path(Path(path_str))


class BaseValueEdit(LineEdit):
    """
    Base line edit for numeric string entry.

    NOTE: This widget implements its own builtin validation rather than
          using a QValidator. This choice was made to improve handling
          of text selection on return pressed in cooperation with being
          driven by a DataWidgetMapper.
    """

    __value_type__ = None
    """Numeric data type to convert input to."""

    value_changed = QtCore.Signal(__value_type__)

    def __init__(
        self,
        default: Optional[__value_type__] = None,
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param default: Default numeric value
        """
        super().__init__(parent=parent)
        self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)

        # Store the last valid state, which will be restored when invalid input is
        # received.
        self._last_valid_value = None

        if default is None:
            default = self.__value_type__()
        else:
            default = self.__value_type__(default)
        self.default = default
        self.set_value(self.default)

        # Connections
        self.editingFinished.connect(self._on_editing_finished)
        self.returnPressed.connect(self._on_return_pressed)

    # Common public interface
    def value(self) -> Any:
        return self.__value_type__(self.text())

    def set_value(self, value: Any, raise_exc: bool = False) -> None:
        """
        :param value: Value to set
        :param raise_exc: Whether to raise an exception if value is
            invalid, instead of falling back to the last valid or
            default value.
        """
        with self._preserve_select_all():
            self.setText(str(value))

        self.validate(raise_exc=raise_exc)

    def reset(self) -> None:
        """Restore default value."""
        self.set_value(self.default)
        self.validate()

    def format(self, value: __value_type__) -> str:
        """
        Subclasses must implement numeric value to string conversion in
        this method.

        :param value: Numeric value
        :return: Formatted value string
        """
        raise NotImplementedError

    def validate(self, raise_exc: bool = False) -> None:
        """
        Validate stored text as a representation of the expected
        numeric type and reformat as needed per the implemented
        ``format`` method.

        :param raise_exc: Whether to raise an exception if value is
            invalid, instead of falling back to the last valid or
            default value.
        """
        try:
            # Try to convert the string to the configured numeric type
            value = self.__value_type__(self.text())
        except (ValueError, TypeError):
            if raise_exc:
                raise

            # Value is invalid. Reset to the last valid or default value, which are
            # guaranteed to be valid. Allow an exception to be raised on the next
            # validate cycle in the off chance the default value is itself invalid.
            if self._last_valid_value is not None:
                self.set_value(self._last_valid_value, raise_exc=True)
            else:
                self.set_value(self.default, raise_exc=True)
            return

        # Value is valid
        self._last_valid_value = value

        # Format new value text
        with self._preserve_select_all():
            self.setText(self.format(value))

    def _on_editing_finished(self) -> None:
        self.validate()
        self.value_changed.emit(self.value())

    def _on_return_pressed(self) -> None:
        # Select all when the user indicates they are done entering a value. This makes
        # it easy to start entering a new value when iterating on a parameter.
        self.selectAll()

    @contextmanager
    def _preserve_select_all(self):
        """
        Context manager which preserves select-all state through value
        changes. This prevents dropping this state when reformatting
        values or receiving model updates.
        """
        select_all = False
        if self.hasSelectedText() and self.selectionLength() == len(self.text()):
            select_all = True

        yield

        if select_all:
            self.selectAll()


class FloatEdit(BaseValueEdit):
    __value_type__ = float

    value_changed = QtCore.Signal(__value_type__)

    def __init__(
        self,
        default: Optional[__value_type__] = None,
        int_reduction: bool = True,
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param default: Default numeric value
        :param int_reduction: When set to True (the default), whole
            number floats are formatted as integers.
        """
        self._int_reduction = int_reduction

        super().__init__(default, parent)

    # DataWidgetMapper user property interface
    @QtCore.Property(float, user=True)
    def __data(self) -> float:
        return self.value()

    @__data.setter
    def __data(self, data: float) -> None:
        with SignalsBlocked(self):
            self.set_value(data)

    def format(self, value: float) -> str:
        # 1.000 -> 1.
        formatted = f"{float(value):.15f}".rstrip("0")
        if self._int_reduction:
            # 1. -> 1
            formatted = formatted.rstrip(".")
        elif formatted.endswith("."):
            # 1. -> 1.0
            formatted += "0"
        return formatted


class IntEdit(BaseValueEdit):
    __value_type__ = int

    value_changed = QtCore.Signal(__value_type__)

    # DataWidgetMapper user property interface
    @QtCore.Property(int, user=True)
    def __data(self) -> int:
        return self.value()

    @__data.setter
    def __data(self, data: int) -> None:
        with SignalsBlocked(self):
            self.set_value(data)

    def format(self, value: float) -> str:
        return str(int(value))


class BaseValueEditArray(QtWidgets.QWidget):
    """Base widget for numeric string array entry."""

    __value_type__: type = None
    """Numeric data type to convert input to."""

    __value_edit_type__: BaseValueEdit = None
    """Value edit widget type to use per array component."""

    value_changed = QtCore.Signal(str, __value_type__)

    LABEL_COLORS = {"r": R_COLOR, "g": G_COLOR, "b": B_COLOR}

    def __init__(
        self,
        labels: Sequence[str],
        defaults: Optional[Sequence[Any]] = None,
        shape: Optional[tuple[int, int]] = None,
        parent: Optional[QtCore.QObject] = None,
    ):
        """
        :param labels: Label per array component in row major order
        :param defaults: Default values, matching label order
        :param shape: Array shape as (columns, rows). If unset, shape
            will default to (label count, 1).
        """
        super().__init__(parent=parent)

        # Labels
        self.labels = labels
        num_labels = len(self.labels)
        has_labels = bool(list(filter(None, self.labels)))

        # Default values
        if defaults is None:
            defaults = [self.__value_type__()] * num_labels
        else:
            num_defaults = len(defaults)
            defaults = [self.__value_type__(v) for v in defaults]
            if num_defaults < num_labels:
                defaults += [self.__value_type__()] * (num_labels - num_defaults)
        self.defaults = defaults

        # 2D array shape
        if shape is None:
            shape = (num_labels, 1)
        self.shape = shape
        columns, rows = self.shape

        # Build array widgets and layout
        self.value_edits = []

        layout = QtWidgets.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)

        for row in range(rows):
            if len(self.value_edits) == num_labels:
                break

            row_layout = QtWidgets.QHBoxLayout()
            row_layout.setContentsMargins(0, 0, 0, 0)
            if not has_labels:
                row_layout.setSpacing(0)

            for column in range(columns):
                if len(self.value_edits) == num_labels:
                    break

                index = row * columns + column
                label = self.labels[index]
                value_label = QtWidgets.QLabel(label)
                if label in self.LABEL_COLORS:
                    value_label.setStyleSheet(
                        f"color: {self.LABEL_COLORS[label].name()};"
                    )

                value_edit = self.__value_edit_type__(default=defaults[index])
                value_edit.value_changed.connect(
                    lambda v, l=label: self.value_changed.emit(l, v)
                )
                self.value_edits.append(value_edit)

                row_layout.addWidget(value_label)
                row_layout.setStretchFactor(value_label, 0)
                row_layout.addWidget(value_edit)
                row_layout.setStretchFactor(value_edit, 1)

            layout.addLayout(row_layout)

        self.setLayout(layout)

    # DataWidgetMapper user property interface
    @QtCore.Property(list, user=True)
    def __data(self) -> list[__value_type__]:
        return self.value()

    @__data.setter
    def __data(self, data: list[__value_type__]) -> None:
        with SignalsBlocked(self):
            self.set_value(data)

    # Common public interface
    def value(self) -> list[__value_type__]:
        """
        :return: list of all array values in row major order
        """
        return [s.value() for s in self.value_edits]

    def set_value(self, values: Sequence[__value_type__]) -> None:
        """
        :param values: Sequence of array values in row major order
        """
        with SignalsBlocked(self, *self.value_edits):
            for i, value in enumerate(values):
                if i < len(self.value_edits):
                    self.value_edits[i].set_value(value)

    def component_value(self, label: str) -> __value_type__:
        """
        :param label: Label of component to get
        :return: Value for one array component. If label is invalid,
            the value type default constructor value is returned
            (usually equivalent to 0).
        """
        value_edit = self._get_value_edit(label)
        if value_edit is not None:
            return value_edit.value()
        return self.__value_type__()

    def set_component_value(self, label: str, value: __value_type__) -> None:
        """
        :param label: Label for array component to set
        :param value: Value to set
        """
        value_edit = self._get_value_edit(label)
        if value_edit is not None:
            value_edit.set_value(value)

    def reset(self, label: Optional[str] = None) -> None:
        """
        Restore default value.

        :param label: Optional label of component to reset. If unset,
            all values will be reset.
        """
        with SignalsBlocked(self, *self.value_edits):
            if label is None:
                for value_edit in self.value_edits:
                    value_edit.reset()
            else:
                value_edit = self._get_value_edit(label)
                if value_edit is not None:
                    value_edit.reset()

    def _get_value_edit(self, label: str) -> Optional[BaseValueEdit]:
        """
        :param label: Label of array component to get widget for
        :return: Value edit widget, or None if no component with label
            was found.
        """
        if label in self.labels:
            return self.value_edits[self.labels.index(label)]
        return None


class FloatEditArray(BaseValueEditArray):
    __value_type__ = float
    __value_edit_type__ = FloatEdit

    value_changed = QtCore.Signal(str, __value_type__)


class IntEditArray(BaseValueEditArray):
    __value_type__ = int
    __value_edit_type__ = IntEdit

    value_changed = QtCore.Signal(str, __value_type__)
