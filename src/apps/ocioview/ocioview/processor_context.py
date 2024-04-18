# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

from dataclasses import dataclass
from typing import Type


@dataclass
class ProcessorContext:
    """
    Data about current config items that constructed a processor.
    """

    input_color_space: str | None
    """Input color space name."""

    transform_item_type: Type | None
    """Config item type for output transform."""

    transform_item_name: str | None
    """Config item name for output transform."""

    inverse: bool = False
    """
    True if the processor is converting from the output transform to 
    the input color space, otherwise the processor is converting from 
    the input color space to the output transform.
    """
