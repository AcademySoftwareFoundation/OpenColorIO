# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# -----------------------------------------------------------------------------
#
# 'HEADER_TEMPLATE', 'CPP_OPERATORS', and 'sanitize_name' are derived from 
# pybind11_mkdoc by Wenzel Jakob, released under The MIT License. 
# 
# See: https://github.com/pybind/pybind11_mkdoc
# 
# This module produces a header matching the results of pybind11_mkdoc, but 
# utilzing Doxygen XML instead of Clang for docstring extraction. The full 
# pybind11_mkdoc license is below:
#
# -----------------------------------------------------------------------------
#
# The MIT License (MIT)
# 
# Copyright (c) 2020 Wenzel Jakob
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# -----------------------------------------------------------------------------

import argparse
import logging
import os
import re
import sys
import time
import xml.etree.ElementTree as ET
from collections import OrderedDict

import six


logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

KINDS_COMPOUND = ("class", "namespace", "struct")
KINDS_MEMBER = ("enum", "function", "variable")

# TODO: Replace RST with doxygen directives in headers and remove this
REMOVE_TOKENS = (
    "rst:: ", 
    "rst::", 
    "cpp:function:: ",
    "cpp:type:: ",
)

HEADER_TEMPLATE = """/*
  This file contains docstrings for use in the Python bindings.
  Do not edit! They were automatically extracted by OpenColorIO/share/docs/extract_docstrings.py.
 */

#define __EXPAND(x)                                      x
#define __COUNT(_1, _2, _3, _4, _5, _6, _7, COUNT, ...)  COUNT
#define __VA_SIZE(...)                                   __EXPAND(__COUNT(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1))
#define __CAT1(a, b)                                     a ## b
#define __CAT2(a, b)                                     __CAT1(a, b)
#define __DOC1(n1)                                       __doc_##n1
#define __DOC2(n1, n2)                                   __doc_##n1##_##n2
#define __DOC3(n1, n2, n3)                               __doc_##n1##_##n2##_##n3
#define __DOC4(n1, n2, n3, n4)                           __doc_##n1##_##n2##_##n3##_##n4
#define __DOC5(n1, n2, n3, n4, n5)                       __doc_##n1##_##n2##_##n3##_##n4##_##n5
#define __DOC6(n1, n2, n3, n4, n5, n6)                   __doc_##n1##_##n2##_##n3##_##n4##_##n5##_##n6
#define __DOC7(n1, n2, n3, n4, n5, n6, n7)               __doc_##n1##_##n2##_##n3##_##n4##_##n5##_##n6##_##n7
#define DOC(...)                                         __EXPAND(__EXPAND(__CAT2(__DOC, __VA_SIZE(__VA_ARGS__)))(__VA_ARGS__))

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

{docs}

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
"""

CPP_OPERATORS = {
     "<=": "le",     ">=": "ge",       "==": "eq",      "!=": "ne", 
     "[]": "array",  "+=": "iadd",     "-=": "isub",    "*=": "imul", 
     "/=": "idiv",   "%=": "imod",     "&=": "iand",    "|=": "ior", 
     "^=": "ixor",  "<<=": "ilshift", ">>=": "irshift", "++": "inc", 
     "--": "dec",    "<<": "lshift",   ">>": "rshift",  "&&": "land", 
     "||": "lor",     "!": "lnot",      "~": "bnot",     "&": "band",  
      "|": "bor",     "+": "add",       "-": "sub",      "*": "mul", 
      "/": "div",     "%": "mod",       "<": "lt",       ">": "gt", 
      "=": "assign", "()": "call",
}
CPP_OPERATORS = OrderedDict(
    sorted(CPP_OPERATORS.items(), key=lambda t: -len(t[0]))
)


def sanitize_name(name):
    # Some Doxygen operators have an extra space:
    #   "operator &&" -> "operator&&"
    name = name.replace("operator ", "operator")

    for k, v in CPP_OPERATORS.items():
        name = name.replace("operator%s" % k, "operator_%s" % v)
    name = re.sub("<.*>", "", name)
    name = "".join([ch if ch.isalnum() else "_" for ch in name])
    name = re.sub("_$", "", re.sub("_+", "_", name))
    return "__doc_" + name


# -----------------------------------------------------------------------------
# XML element handling and traversal
# -----------------------------------------------------------------------------

class MetaElementRenderer(type):
    """
    Meta class that auto-registers ElementRender subclasses.
    """
    def __init__(cls, name, bases, attrs):
        if "ElementRenderer" in [b.__name__ for b in bases]:
            # Subclass name == element tag
            ElementRenderer.tag_registry[name] = cls


@six.add_metaclass(MetaElementRenderer)
class ElementRenderer(object):
    """
    Recursively parses and renders a Doxygen XML description element 
    into an RST docstring.
    """
    # Subclasses registered by Doxygen XML tag name
    tag_registry = {}

    # Accumulates all unregistered Doxygen XML tags 
    unregistered_tags = set()

    def __new__(cls, elem, parent=None):
        # Swap in a subclass registered for this tag
        if elem.tag in cls.tag_registry:
            return super(ElementRenderer, cls).__new__(
                cls.tag_registry[elem.tag]
            )
        else:
            cls.unregistered_tags.add(elem.tag)
            return super(ElementRenderer, cls).__new__(cls)

    def __init__(self, elem, parent=None):
        self.elem = elem
        self.parent = parent

    def open(self, result):
        pass

    def body(self, result):
        if self.elem.text:
            lines = []
            for line in self.elem.text.splitlines():
                if line.strip():
                    lines.append(line)
            text = "\n".join(lines)

            for t in REMOVE_TOKENS:
                text = text.replace(t, "")

            return text

    def close(self, result):
        pass

    def tail(self, result):
        if self.elem.tail:
            lines = []
            for line in self.elem.tail.splitlines():
                if line.strip():
                    lines.append(line)
            tail = "\n".join(lines)

            for t in REMOVE_TOKENS:
                tail = tail.replace(t, "")

            return tail

    def parent_types(self):
        types = []
        parent = self.parent
        while parent:
            types.append(parent.__class__)
            parent = parent.parent
        return types

    def get_first_parent(self, parent_type):
        parent = self.parent
        while parent:
            if isinstance(parent, parent_type):
                return parent
            parent = parent.parent
        return None

    @classmethod
    def render(cls, elem, parent=None):
        result = ""
        visitor = cls(elem, parent=parent)

        # "<tag>text</tag>tail"
        #  -----
        result += visitor.open(result) or ""
        # "<tag>text</tag>tail"
        #       ----
        result += visitor.body(result) or ""

        for child in elem:
            result += cls.render(child, parent=visitor)

        # "<tag>text</tag>tail"
        #           ------
        result += visitor.close(result) or ""

        # "<tag>text</tag>tail"
        #                 ----
        result += visitor.tail(result) or ""

        return result


# -----------------------------------------------------------------------------
# Doxygen XML element rendering
# -----------------------------------------------------------------------------

class briefdescription(ElementRenderer):
    pass


class detaileddescription(ElementRenderer):
    pass   


class bold(ElementRenderer):
    def open(self, result):
        return "**"

    def close(self, result):
        return "**"


class emphasis(ElementRenderer):
    def open(self, result):
        return "*"

    def close(self, result):
        return "*"


class simplesect(ElementRenderer):
    def open(self, result):
        return ".. {kind}::\n   ".format(kind=self.elem.attrib["kind"])


class programlisting(ElementRenderer):
    def open(self, result):
        return ".. code-block:: cpp\n\n"

    def close(self, result):
        return "\n"


class codeline(ElementRenderer):
    def open(self, result):
        return "    "

    def close(self, result):
        return "\n"


class highlight(ElementRenderer):
    def open(self, result):
        if programlisting not in self.parent_types():
            return "``"

    def close(self, result):
        if programlisting not in self.parent_types():
            return "``"


class verbatim(ElementRenderer):
    def open(self, result):
        return "\n\n    "

    def body(self, result):
        if self.elem.text:
            return self.elem.text.replace("\n", "\n    ")

    def close(self, result):
        return "\n"


class itemizedlist(ElementRenderer):
    def open(self, result):
        return "\n"


class listitem(ElementRenderer):
    def open(self, result):
        return "- "


class parameterlist(ElementRenderer):
    def open(self, result):
        return "\n"
        
    def close(self, result):
        return "\n"


class parameteritem(ElementRenderer):
    pass


class parameternamelist(ElementRenderer):
    pass


class parametername(ElementRenderer):
    def open(self, result):
        kind = "param"
        parent = self.get_first_parent(parameterlist)
        if parent:
            kind = parent.elem.attrib["kind"]

        return ":{kind} ".format(kind=kind)

    def close(self, result):
        return ": "


class parameterdescription(ElementRenderer):
    pass


class para(ElementRenderer):
    def close(self, result):
        if any(t in self.parent_types() for t in (listitem, parameteritem)):
            return "\n"
        else:
            return "\n\n"


class computeroutput(ElementRenderer):
    def open(self, result):
        return "`"

    def close(self, result):
        return "`"


class ref(ElementRenderer):
    def open(self, result):
        return ":ref:`"

    def close(self, result):
        return "`"


class heading(ElementRenderer):
    LEVELS = ("=", "*", "+", "^", "-", "#")

    def close(self, result):
        level = int(self.elem.attrib["level"])
        return "\n" + self.LEVELS[level-1] * len(result) + "\n"


class ndash(ElementRenderer):
    def body(self, result):
        return "--"


class sp(ElementRenderer):
    def body(self, result):
        return " "


# -----------------------------------------------------------------------------
# Doxygen XML to header conversion
# -----------------------------------------------------------------------------

def render_docstring(def_elem, name, names):
    """
    Render single docstring definition source.
    """
    # Handle brief and/or detailed description elements
    value = "\n".join(filter(None, [
        ElementRenderer.render(def_elem.find("briefdescription")),
        ElementRenderer.render(def_elem.find("detaileddescription"))
    ])).lstrip("\n").rstrip()

    # Clean up excessive newlines
    value = re.sub(r"\n{3,}", "\n\n", value)

    # Increment duplicate names, accounting for overloaded functions
    var_name = orig_var_name = sanitize_name(name)
    i = 2
    while var_name in names:
        var_name = "{var_name}_{i}".format(var_name=orig_var_name, i=i)
        i += 1
    names.add(var_name)

    return "static const char *{name} ={sep}R\"doc({doc})doc\";".format(
        name=var_name,
        sep="\n" if "\n" in value else " ",
        doc=value
    )


def extract_docstrings(args):
    """
    Extract all docstrings and write header.
    """
    start = time.time()
    xml_count = 0
    names = set()
    docs = []

    logger.info(
        "Extracting docstrings from Doxygen XML: {path}".format(
            path=args.xmldir
        )
    )

    # Walk doxygen XML files for docstrings
    for filename in os.listdir(args.xmldir):
        if os.path.splitext(filename)[1] == ".xml":
            xml_count += 1
            logger.info("Processing file: {path}".format(path=filename))

            tree = ET.parse(os.path.join(args.xmldir, filename))
            root = tree.getroot()
            
            # Compound object/namespace
            for compounddef_elem in root.iter("compounddef"):
                if compounddef_elem.attrib["kind"] not in KINDS_COMPOUND:
                    continue

                compound_name = compounddef_elem.find("compoundname").text
                if "::" not in compound_name:
                    # Generalize OCIO namespace
                    compound_name = "PyOpenColorIO"
                    
                compound_full_name = compound_name.split("::")[-1]

                logger.debug(
                    "Converting compound definition: {name}".format(
                        name=compound_name
                    )
                )

                compound_doc = render_docstring(
                    compounddef_elem,
                    compound_full_name,
                    names
                )
                docs.append(compound_doc)

                # Compound member
                for memberdef_elem in compounddef_elem.iter("memberdef"):
                    if memberdef_elem.attrib["kind"] not in KINDS_MEMBER:
                        continue

                    member_name = memberdef_elem.find("name").text
                    if (compound_name == "PyOpenColorIO" and
                            "operator" in member_name):
                        # Insert return type to distinguish OCIO namespace 
                        # operators. Argument types may be needed in the 
                        # future as well.
                        member_name = "_".join([
                            memberdef_elem.find("type").text, 
                            member_name
                        ])

                    member_full_name = "{compound}_{member}".format(
                        compound=compound_full_name, 
                        member=member_name
                    )

                    logger.debug(
                        "Converting member definition: {name}".format(
                            name=member_name
                        )
                    )

                    member_doc = render_docstring(
                        memberdef_elem,
                        member_full_name,
                        names
                    )
                    docs.append(member_doc)

                    # Enum value
                    for enumvalue_elem in memberdef_elem.iter("enumvalue"):
                        value_name = enumvalue_elem.find("name").text

                        value_full_name = "{member}_{value}".format(
                            member=member_full_name,
                            value=value_name
                        )

                        logger.debug(
                            "Converting enum value definition: {name}".format(
                                name=value_name
                            )
                        )

                        enumvalue_doc = render_docstring(
                            enumvalue_elem,
                            value_full_name,
                            names
                        )
                        docs.append(enumvalue_doc)

    # Write docstring header
    logger.info("Writing docstring header: {path}".format(path=args.header))
    with open(args.header, "w") as header_file:
        header_file.write(HEADER_TEMPLATE.format(docs="\n\n".join(docs)))

    logger.info(
        "{doc_count:d} docstrings extracted from {xml_count:d} XML files in "
        "{duration:.2f} seconds".format(
            doc_count=len(docs),
            xml_count=xml_count,
            duration=time.time() - start
        )
    )

    # Raise on any unhandled tags (to break CI build), forcing up-to-date 
    # element handling.
    if ElementRenderer.unregistered_tags:
        raise RuntimeError(
            "{count} element types unhandled while parsing Doxygen XML: "
            "'{tags}'".format(
                count=len(ElementRenderer.unregistered_tags),
                tags="', '".join(sorted(ElementRenderer.unregistered_tags))
            )
        )


# -----------------------------------------------------------------------------
# Commandline interface
# -----------------------------------------------------------------------------

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert a directory containing Doxygen XML output into a "
                    "single C++ header matching the output and purpose of "
                    "pybind11_mkdoc, containing extracted RST docstrings to "
                    "reference in pybind11 bindings."
    )
    parser.add_argument(
        "xmldir", 
        help="Path to directory containing one or more Doxygen XML files."
    )
    parser.add_argument(
        "header", 
        help="Path to destination header file to be generated."
    )

    try:
        extract_docstrings(parser.parse_args())
        sys.exit(0)
    except Exception as e:
        logger.error(str(e), exc_info=e)
        sys.exit(1)
