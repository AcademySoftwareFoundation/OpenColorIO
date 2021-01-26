# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# -----------------------------------------------------------------------------
# 
# Adapted from sphinxcontrib.prettyspecialmethods by Thomas Smith, released 
# under The MIT License. 
# 
# See: https://github.com/sphinx-contrib/prettyspecialmethods
# 
# Shows special methods as the python syntax that invokes them. Support has 
# been added for named constructors and pybind11 generated docstrings which
# include the 'self' argumnt in all methods. The full prettyspecialmethods 
# license is below:
#
# -----------------------------------------------------------------------------
#
# MIT License
#
# Copyright (c) 2018 Thomas Smith
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# -----------------------------------------------------------------------------

import sphinx.addnodes as SphinxNodes
from docutils.nodes import Text, emphasis, inline
from sphinx.transforms import SphinxTransform


__version__ = "0.1.0"


def is_method(node):
    return node and "self" in node.astext()


def get_children(parameters_node):
    if is_method(parameters_node):
        # Remove 'self' parameter
        return parameters_node.children[1:]
    else:
        return parameters_node.children


def patch_node(node, text=None, children=None, *, constructor=None):
    if constructor is None:
        constructor = node.__class__

    if text is None:
        text = node.text

    if children is None:
        children = get_children(node)

    return constructor(
        node.source,
        text,
        *children,
        **node.attributes,
    )


def function_transformer(new_name):
    def xf(name_node, parameters_node, details):
        return (
            patch_node(name_node, new_name, ()),
            patch_node(parameters_node, "", [
                SphinxNodes.desc_parameter("", "self"),
                *get_children(parameters_node),
            ])
        )

    return xf


def unary_op_transformer(op):
    def xf(name_node, parameters_node, details):
        return (
            patch_node(name_node, op, ()),
            emphasis("", "self"),
        )

    return xf


def binary_op_transformer(op):
    def xf(name_node, parameters_node, details):
        return inline(
            "", "",
            emphasis("", "self"),
            Text(" "),
            patch_node(name_node, op, ()),
            Text(" "),
            emphasis("", get_children(parameters_node)[0].astext())
        )

    return xf


def brackets(parameters_node):
    return [
        emphasis("", "self"),
        SphinxNodes.desc_name("", "", Text("[")),
        emphasis("", get_children(parameters_node)[0].astext()),
        SphinxNodes.desc_name("", "", Text("]")),
    ]


SPECIAL_METHODS = {
    # Use class-named constructor
    "__init__": lambda name_node, parameters_node, details: inline(
        "", "",
        SphinxNodes.desc_name(
            "", "", 
            Text(details.get("class_name", name_node.astext()))
        ),
        patch_node(parameters_node, "", get_children(parameters_node))
    ),

    "__getitem__": lambda name_node, parameters_node, details: inline(
        "", "", *brackets(parameters_node)
    ),
    "__setitem__": lambda name_node, parameters_node, details: inline(
        "", "",
        *brackets(parameters_node),
        Text(" "),
        SphinxNodes.desc_name("", "", Text("=")),
        Text(" "),
        emphasis("", (
            (get_children(parameters_node)[1].astext())
            if len(get_children(parameters_node)) > 1 else ""
        )),
    ),
    "__delitem__": lambda name_node, parameters_node, details: inline(
        "", "",
        SphinxNodes.desc_name("", "", Text("del")),
        Text(" "),
        *brackets(parameters_node),
    ),
    "__contains__": lambda name_node, parameters_node, details: inline(
        "", "",
        emphasis("", parameters_node.children[1].astext()),
        Text(" "),
        SphinxNodes.desc_name("", "", Text("in")),
        Text(" "),
        emphasis("", "self"),
    ),

    "__await__": lambda name_node, parameters_node, details: inline(
        "", "",
        SphinxNodes.desc_name("", "", Text("await")),
        Text(" "),
        emphasis("", "self"),
    ),

    "__lt__": binary_op_transformer("<"),
    "__le__": binary_op_transformer("<="),
    "__eq__": binary_op_transformer("=="),
    "__ne__": binary_op_transformer("!="),
    "__gt__": binary_op_transformer(">"),
    "__ge__": binary_op_transformer(">="),

    "__hash__": function_transformer("hash"),
    "__len__": function_transformer("len"),
    "__iter__": function_transformer("iter"),
    "__next__": function_transformer("next"), 
    "__str__": function_transformer("str"),
    "__repr__": function_transformer("repr"),

    "__add__": binary_op_transformer("+"),
    "__sub__": binary_op_transformer("-"),
    "__mul__": binary_op_transformer("*"),
    "__matmul__": binary_op_transformer("@"),
    "__truediv__": binary_op_transformer("/"),
    "__floordiv__": binary_op_transformer("//"),
    "__mod__": binary_op_transformer("%"),
    "__divmod__": function_transformer("divmod"),
    "__pow__": binary_op_transformer("**"),
    "__lshift__": binary_op_transformer("<<"),
    "__rshift__": binary_op_transformer(">>"),
    "__and__": binary_op_transformer("&"),
    "__xor__": binary_op_transformer("^"),
    "__or__": binary_op_transformer("|"),

    "__neg__": unary_op_transformer("-"),
    "__pos__": unary_op_transformer("+"),
    "__abs__": function_transformer("abs"),
    "__invert__": unary_op_transformer("~"),

    "__call__": lambda name_node, parameters_node, details: (
        emphasis("", "self"),
        patch_node(parameters_node, "", get_children(parameters_node))
    ),
    "__getattr__": function_transformer("getattr"),
    "__setattr__": function_transformer("setattr"),
    "__delattr__": function_transformer("delattr"),

    "__bool__": function_transformer("bool"),
    "__int__": function_transformer("int"),
    "__float__": function_transformer("float"),
    "__complex__": function_transformer("complex"),
    "__bytes__": function_transformer("bytes"),

    # could show this as "{:...}".format(self) if we wanted
    "__format__": function_transformer("format"),

    "__index__": function_transformer("operator.index"),
    "__length_hint__": function_transformer("operator.length_hint"),
    "__ceil__": function_transformer("math.ceil"),
    "__floor__": function_transformer("math.floor"),
    "__trunc__": function_transformer("math.trunc"),
    "__round__": function_transformer("round"),

    "__sizeof__": function_transformer("sys.getsizeof"),
    "__dir__": function_transformer("dir"),
    "__reversed__": function_transformer("reversed"),
}


class PrettifyMethods(SphinxTransform):
    default_priority = 800

    def apply(self):
        methods = (
            sig for sig in self.document.traverse(SphinxNodes.desc_signature)
            if "class" in sig
        )

        # Extra data for replace functions
        details = {}

        for ref in methods:
            name_node = ref.next_node(SphinxNodes.desc_name)
            name = name_node.astext()

            # Determine the class to associate with this method, so we can
            # swap __init__ with a named constructor.
            annotation_node = ref.next_node(SphinxNodes.desc_annotation)
            if annotation_node:
                annotation = annotation_node.astext().strip()
                if annotation == "class":
                    details["class_name"] = name

            parameters_node = ref.next_node(SphinxNodes.desc_parameterlist)

            # Transform special methods
            if name in SPECIAL_METHODS:
                name_node.replace_self(
                    SPECIAL_METHODS[name](name_node, parameters_node, details)
                )
                parameters_node.replace_self(())
            
            # Remove 'self' parameter from other methods
            elif is_method(parameters_node):
                parameters_node.replace_self(patch_node(parameters_node, ""))


def setup(app):
    app.add_transform(PrettifyMethods)
    app.setup_extension("sphinx.ext.autodoc")
    return {"version": __version__, "parallel_read_safe": True}
