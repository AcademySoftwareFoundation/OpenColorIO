# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import re

import PyOpenColorIO as ocio
from pygments import highlight
from pygments.lexers import GLShaderLexer, HLSLShaderLexer, XmlLexer, YamlLexer
from pygments.formatters import HtmlFormatter


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
