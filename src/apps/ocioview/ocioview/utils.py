# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import enum
import re
from pathlib import Path
from types import TracebackType
from typing import Optional, Union

import PyOpenColorIO as ocio
import qtawesome
from pygments import highlight
from pygments.lexers import GLShaderLexer, HLSLShaderLexer, XmlLexer, YamlLexer
from pygments.formatters import HtmlFormatter
from PySide6 import QtCore, QtGui, QtWidgets

from .constants import ICON_SCALE_FACTOR, ICON_SIZE_BUTTON


class SignalsBlocked:
    """
    Context manager which blocks signals to supplied QObjects during
    execution of the contained scope.
    """

    def __init__(self, *args: QtCore.QObject):
        self.objects = list(args)

    def __enter__(self) -> None:
        for obj in self.objects:
            obj.blockSignals(True)

    def __exit__(
        self, exc_type: type, exc_val: Exception, exc_tb: TracebackType
    ) -> None:
        for obj in self.objects:
            obj.blockSignals(False)


def get_glyph_icon(
    name: str,
    scale_factor: float = ICON_SCALE_FACTOR,
    color: Optional[QtGui.QColor] = None,
    as_widget: bool = False,
) -> Union[QtGui.QIcon, QtWidgets.QLabel]:
    """
    Get named glyph QIcon from QtAwesome.

    :param name: Glyph name
    :param scale_factor: Amount to scale icon
    :param color: Optional icon color override
    :param as_widget: Set to True to return a widget displaying the
        icon instead of a QIcon.
    :return: Glyph QIcon or QLabel
    """
    kwargs = {"scale_factor": scale_factor}
    if color is not None:
        kwargs["color"] = color

    icon = qtawesome.icon(name, **kwargs)

    if as_widget:
        widget = QtWidgets.QLabel()
        widget.setPixmap(icon.pixmap(ICON_SIZE_BUTTON))
        return widget
    else:
        return icon


def get_icon(
    icon_path: Path, rotate: float = 0.0, as_pixmap: bool = False
) -> QtGui.QIcon:
    """
    Get QIcon or QPixmap from an icon path.

    :param icon_path: Icon file path
    :param rotate: Optional degrees to rotate icon
    :param as_pixmap: Whether to return a QPixmap instead of a QIcon
    :return: QIcon or QPixmap
    """
    # Rotate icon?
    if rotate:
        xform = QtGui.QTransform()
        xform.rotate(rotate)
        pixmap = QtGui.QPixmap(str(icon_path))
        pixmap = pixmap.transformed(xform)
        if as_pixmap:
            return pixmap
        else:
            return QtGui.QIcon(pixmap)
    else:
        return [QtGui.QIcon, QtGui.QPixmap][int(as_pixmap)](str(icon_path))


def get_enum_member(enum_type: enum.Enum, value: int) -> Optional[enum.Enum]:
    """
    Lookup an enum member from its type and value.

    :param enum_type: Enum type
    :param value: Enum value
    :return: Enum member or None if value not found
    """
    for member in enum_type.__members__.values():
        if member.value == value:
            return member
    return None


def next_name(prefix: str, all_names: list[str]) -> str:
    """
    Increment a name with the provided prefix and a number suffix until
    it is unique in the given list of names.

    :param prefix: Name prefix, typically followed by an underscore
    :param all_names: All sibling names which a new name cannot
        intersect.
    :return: Unique name
    """
    lower_names = [name.lower() for name in all_names]
    i = 1
    name = prefix + str(i)
    while name.lower() in lower_names:
        i += 1
        name = prefix + str(i)
    return name


def item_type_label(item_type: type) -> str:
    """
    Convert a config item type to a friendly type name
    (e.g. "ColorSpace" -> "Color Space").

    :param item_type: Config item type
    :return: Friendly type name
    """
    return " ".join(filter(None, re.split(r"([A-Z]+[a-z]+)", item_type.__name__)))


def m44_to_m33(m44: list) -> list:
    """
    Convert list with 16 elements representing a 4x4 matrix to a list
    with 9 elements representing a 3x3 matrix.
    """
    return [*m44[0:3], *m44[4:7], *m44[8:11]]


def m33_to_m44(m33: list) -> list:
    """
    Convert list with 9 elements representing a 3x3 matrix to a list
    with 16 elements representing a 4x4 matrix.
    """
    return [*m33[0:3], 0, *m33[3:6], 0, *m33[6:9], 0, 0, 0, 0, 1]


def v4_to_v3(v4: list) -> list:
    """
    Convert list with 4 elements representing an XYZW or RGBA vector to
    a list with 3 elements representing an XYZ or RGB vector.
    """
    return v4[:3]


def v3_to_v4(v3: list) -> list:
    """
    Convert list with 3 elements representing an XYZ or RGB vector to
    a list with 4 elements representing an XYZW or RGBA vector.
    """
    return [*v3, 0]


def config_to_html(config: ocio.Config) -> str:
    """Return OCIO config formatted as HTML."""
    yaml_data = str(config)
    return increase_html_lineno_padding(
        highlight(yaml_data, YamlLexer(), HtmlFormatter(linenos="inline"))
    )


def processor_to_ctf_html(processor: ocio.Processor) -> tuple[str, ocio.GroupTransform]:
    """Return processor CTF formatted as HTML."""
    config = ocio.GetCurrentConfig()
    group_tf = processor.createGroupTransform()

    # Replace LUTs with identity LUTs since formatting and printing LUT data
    # is expensive and unnecessary unless we're exporting CTF data to a file.
    clean_group_tf = ocio.GroupTransform()
    for tf in group_tf:
        if isinstance(tf, ocio.Lut1DTransform):
            clean_tf = ocio.Lut1DTransform()
        elif isinstance(tf, ocio.Lut3DTransform):
            clean_tf = ocio.Lut3DTransform()
        else:
            clean_tf = tf
        clean_group_tf.appendTransform(clean_tf)

    ctf_data = clean_group_tf.write("Color Transform Format", config)

    # Inject ellipses into LUT elements to indicate to viewers that the data
    # is truncated.
    ctf_data = re.sub(
        r"(<LUT[13]D)[\s\S]*?(</LUT[13]D>)",
        r"\1>...[Export CTF to include LUT]...\2",
        ctf_data,
    )

    return (
        increase_html_lineno_padding(
            highlight(ctf_data, XmlLexer(), HtmlFormatter(linenos="inline"))
        ),
        group_tf,
    )


def processor_to_shader_html(
    gpu_proc: ocio.GPUProcessor, gpu_language: ocio.GPU_LANGUAGE_GLSL_4_0
) -> str:
    """
    Return processor shader in the requested language, formatted as
    HTML.
    """
    gpu_shader_desc = ocio.GpuShaderDesc.CreateShaderDesc(language=gpu_language)
    gpu_proc.extractGpuShaderInfo(gpu_shader_desc)
    shader_data = gpu_shader_desc.getShaderText()

    return increase_html_lineno_padding(
        highlight(
            shader_data,
            (GLShaderLexer if "GLSL" in gpu_language.name else HLSLShaderLexer)(),
            HtmlFormatter(linenos="inline"),
        )
    )


def increase_html_lineno_padding(html: str) -> str:
    """
    Adds two non-breaking spaces to the right of all line numbers
    for some breathing room around the code.
    """
    # This works with inline and table linenos
    return re.sub(
        r"(<span class=\"(?:normal|special|linenos)\">\s*)([0-9]+)(</span>)",
        r"\1\2&nbsp;&nbsp;\3",
        html,
    )
