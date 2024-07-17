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
    """
    Signal that is emitted whenever the current application mode is 
    changed.
    """

    config_changed = QtCore.Signal()
    """
    Signal that is emitted whenever the current config is modified or 
    replaced.
    """

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

    def emit_config_changed(self) -> None:
        """
        Notify listeners that the current OCIO config has been modified
        or replaced.
        """
        self.config_changed.emit()

    def emit_mode_changed(self) -> None:
        """
        Notify listeners that the current application mode has changed.
        """
        self.mode_changed.emit()
