# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

from typing import Optional

from PySide6 import QtCore


class SignalRouter(QtCore.QObject):
    """
    Singleton router which routes application-wide signals to
    listeners.
    """

    mode_changed = QtCore.Signal()
    """Emitted when the current application mode is changed."""

    config_changed = QtCore.Signal()
    """Emitted when the current config is modified."""

    config_reloaded = QtCore.Signal()
    """Emitted when the current config is reloaded or replaced."""

    color_spaces_changed = QtCore.Signal()
    """Emitted when a color space is added, removed, or changed."""

    roles_changed = QtCore.Signal()
    """Emitted when a color space role is added, removed, or changed."""

    __instance: SignalRouter = None

    @classmethod
    def get_instance(cls) -> SignalRouter:
        """Get singleton SignalRouter instance."""
        if cls.__instance is None:
            cls.__instance = SignalRouter()
        return cls.__instance

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Only allow __init__ to be called once
        if self.__instance is not None:
            raise RuntimeError(
                f"{self.__class__.__name__} is a singleton. Please call "
                f"'get_instance' to access this type."
            )
        else:
            self.__instance = self

    def emit_mode_changed(self) -> None:
        """
        Notify listeners that the current application mode has changed.
        """
        self.mode_changed.emit()

    def emit_config_changed(self) -> None:
        """
        Notify listeners that the current OCIO config has been modified.
        """
        self.config_changed.emit()

    def emit_config_reloaded(self) -> None:
        """
        Notify listeners that the current OCIO config has been reloaded
        or replaced and changed.
        """
        self.config_reloaded.emit()
        self.emit_config_changed()

    def emit_color_spaces_changed(self) -> None:
        """
        Notify listeners when a color space is added, removed, or
        changed.
        """
        self.color_spaces_changed.emit()

    def emit_roles_changed(self) -> None:
        """
        Notify listeners when a color space role is added, removed, or
        changed.
        """
        self.roles_changed.emit()
