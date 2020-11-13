# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# -----------------------------------------------------------------------------
#
# This Sphinx monkeypatch emits autodoc generated RST for use in an environment 
# where the dependent Python package is unavailable (e.g. Python bindings which 
# must be compiled, but can"t due to a build environment like readthedocs.org).
#
# The solution is based on two Stack Overflow answers, which point out an entry
# point in autodoc for capturing generated RST:
#
#  Answer: https://stackoverflow.com/a/2712413
#  Author: Michal Čihař (https://stackoverflow.com/users/225718/michal-%c4%8ciha%c5%99)
# License: CC BY-SA 4.0 (https://creativecommons.org/licenses/by-sa/4.0/)
#
#  Answer: https://stackoverflow.com/a/31648880
#  Author: srepmub (https://stackoverflow.com/users/1368566/srepmub)
# License: CC BY-SA 3.0 (https://creativecommons.org/licenses/by-sa/3.0/)
#
# This module extends the sphinx.ext.autodoc.Documenter.add_line method defined 
# in this source file:
#   https://github.com/sphinx-doc/sphinx/blob/3.x/sphinx/ext/autodoc/__init__.py
#
# Sphinx is released under the BSD license. The full license contents are 
# below:
#
# -----------------------------------------------------------------------------
#
# Copyright (c) 2007-2020 by the Sphinx team (see AUTHORS file).
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# -----------------------------------------------------------------------------

import os
import shutil

import sphinx.ext.autodoc


MODULE = os.path.realpath(__file__)
HERE = os.path.dirname(MODULE)
ROOT = os.path.abspath(os.path.join(HERE, os.pardir, os.pardir))
MODULE_RELPATH = MODULE[len(ROOT)+1:]
DOCS_DIR = os.path.join(ROOT, "docs")
PYTHON_GEN_DIR = os.path.join(DOCS_DIR, "api", "python", "generated")


class RSTRouter(object):
    """
    Manages state for multiple incrementally generated RST files.
    """
    _init_dest_dir = set()
    _current_fullname = None
    _current_rst_file = None

    @classmethod
    def init_dest_dir(cls, dest_dir):
        """
        Re-initialize RST destination dir once per session.
        """
        if dest_dir not in cls._init_dest_dir:
            if os.path.exists(dest_dir):
                shutil.rmtree(dest_dir)
            os.makedirs(dest_dir)
            cls._init_dest_dir.add(dest_dir)

    @classmethod
    def init_rst_file(cls, dest_dir, fullname, after_token, header):
        """
        Return an RST file object for capturing RST lines.
        """
        cls.init_dest_dir(dest_dir)

        if fullname != cls._current_fullname:
            if cls._current_rst_file:
                cls._current_rst_file.close()

            # Given a Python signature, remove all tokens beyond 
            # after_token + 1:
            #   fullname: "a.b.c.d"
            #   after_token: "b"
            #   rst_path: "dest_dir/a.b.c.rst"
            name_tokens = fullname.split(".")
            after_index = name_tokens.index(after_token)
            basename = ".".join(name_tokens[:after_index+2])
            rst_path = os.path.join(PYTHON_GEN_DIR, basename + ".rst")
            rst_exists = os.path.exists(rst_path)

            cls._current_rst_file = open(rst_path, "a" if rst_exists else "w")
            cls._current_fullname = fullname

            # Start each new RST file with the given header
            if not rst_exists:
                cls._current_rst_file.write(header)

        return cls._current_rst_file 


def add_line(self, line, source, *lineno):
    # This block should match the Sphinx source:
    # -------------------------------------------------------------------------
    if line.strip():  # not a blank line
        line_out = self.indent + line
    else:
        line_out = ""
    # -------------------------------------------------------------------------

    rst_file = RSTRouter.init_rst_file(
        PYTHON_GEN_DIR, 
        self.fullname, 
        "PyOpenColorIO",
        "..\n"
        "  SPDX-License-Identifier: CC-BY-4.0\n"
        "  Copyright Contributors to the OpenColorIO Project.\n"
        "  Do not edit! This file was automatically generated by "
        "{module}\n".format(module=MODULE_RELPATH)
    )
    rst_file.write(line_out + "\n")
    # The last rst file won't be explicitly closed, so write to file now
    rst_file.flush()

    # This block should match the Sphinx source:
    # -------------------------------------------------------------------------
    self.directive.result.append(line_out, source, *lineno)
    # -------------------------------------------------------------------------


def patch():
    """
    Apply monkeypatch to autodoc.
    """
    sphinx.ext.autodoc.Documenter.add_line = add_line
