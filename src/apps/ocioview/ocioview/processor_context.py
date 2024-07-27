# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Type

import PyOpenColorIO as ocio


@dataclass
class ProcessorContext:
    """
    Data about current config items that constructed a processor.
    """

    input_color_space: str | None = field(default_factory=lambda: None)
    """Input color space name."""

    transform_item_type: Type | None = field(default_factory=lambda: None)
    """Transform source config item type."""

    transform_item_name: str | None = field(default_factory=lambda: None)
    """Transform source config item name."""

    transform_direction: ocio.TransformDirection = ocio.TRANSFORM_DIR_FORWARD
    """Transform direction being viewed."""
