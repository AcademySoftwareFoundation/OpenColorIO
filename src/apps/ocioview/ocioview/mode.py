# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

import enum


class OCIOViewMode(enum.Enum):
    """
    ocioview application mode enum.

    This class also manages the current application mode, which
    components will refer to for mode-specific layout and behavior.
    """

    Edit = "mdi6.file-document-edit-outline"
    """Mode for editing and inspecting an OCIO config."""

    Preview = "mdi6.television-play"
    """
    Mode for previewing an OCIO config's user experience in a reference 
    integration.
    """

    __current = None
    """Current application mode."""

    @classmethod
    def current_mode(cls) -> OCIOViewMode:
        """Get the current application mode."""
        return cls.__current

    @classmethod
    def set_current_mode(cls, mode: OCIOViewMode) -> None:
        """Set the current application mode."""
        if mode != cls.__current:
            from .signal_router import SignalRouter

            cls.__current = mode

            signal_router = SignalRouter.get_instance()
            signal_router.emit_mode_changed()


OCIOViewMode.set_current_mode(OCIOViewMode.Edit)
