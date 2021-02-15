# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# -----------------------------------------------------------------------------
#
# This Sphinx extension emits "frozen" autodoc generated RST for use in an 
# environment where the dependent Python package is unavailable (e.g. Python 
# bindings which must be compiled, but can't due to a build environment like 
# readthedocs.org). It also facilitates comparison between versions of generated 
# RST to determine when re-freezing is needed to keep up with upstream changes
# which affect autodoc output.
#
# The add_line monkeypatch contained in this module is derived from two Stack 
# Overflow answers, which point out an entry point in autodoc for capturing 
# generated RST:
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

import difflib
import filecmp
import logging
import os
import shutil

import sphinx.ext.autodoc
from sphinx.errors import ExtensionError


logging.basicConfig(
    level=logging.INFO,
    format="[%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger(__name__)

MODULE = os.path.realpath(__file__)
HERE = os.path.dirname(MODULE)
ROOT = os.path.abspath(os.path.join(HERE, os.pardir, os.pardir))
MODULE_RELPATH = MODULE[len(ROOT)+1:].replace("\\", "/")
DOCS_DIR = os.path.join(ROOT, "docs")
PYTHON_FROZEN_DIR = os.path.join(DOCS_DIR, "api", "python", "frozen")
PYTHON_BACKUP_DIR = os.path.join(DOCS_DIR, "api", "python", "backup")


class RSTRouter(object):
    """
    Manages state for multiple incrementally generated RST files.
    """
    _init_dest_dir = set()
    _init_rst_file = set()
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
            basename = "_".join(name_tokens[:after_index+2]).lower()
            rst_path = os.path.join(PYTHON_FROZEN_DIR, basename + ".rst")

            mode = "w"
            if rst_path in cls._init_rst_file:
                mode = "a"

            cls._current_rst_file = open(rst_path, mode, encoding="utf-8")
            cls._current_fullname = fullname

            # Start each new RST file with the given header
            if mode == "w":
                cls._current_rst_file.write(header)
                cls._init_rst_file.add(rst_path)

        return cls._current_rst_file 


def add_line(self, line, source, *lineno):
    if line.strip():  # not a blank line
        line_out = self.indent + line
    else:
        line_out = ""

    rst_file = RSTRouter.init_rst_file(
        PYTHON_FROZEN_DIR, 
        self.fullname, 
        "PyOpenColorIO",
        "..\n"
        "  SPDX-License-Identifier: CC-BY-4.0\n"
        "  Copyright Contributors to the OpenColorIO Project.\n"
        "  Do not edit! This file was automatically generated by "
        "{module}.\n".format(module=MODULE_RELPATH)
    )

    rst_file.write(line_out + "\n")
    # The last rst file won't be explicitly closed, so write to file now
    rst_file.flush()

    self.directive.result.append(line_out, source, *lineno)


def patch():
    """
    Apply monkeypatch to autodoc.
    """
    sphinx.ext.autodoc.Documenter.add_line = add_line


def backup_frozen(app):
    """
    Duplicate existing frozen RST files for comparison with newly 
    frozen RST in ``compare_frozen()`` after docs are built.
    """
    if app.config.frozendoc_build or app.config.frozendoc_compare:
        # Apply monkeypatch, enabling frozen RST generation
        patch()

    if not app.config.frozendoc_compare:
        return

    logger.info("Backing up frozen RST: {src} -> {dst}".format(
        src=PYTHON_FROZEN_DIR, 
        dst=PYTHON_BACKUP_DIR
    ))

    if os.path.exists(PYTHON_FROZEN_DIR) and os.listdir(PYTHON_FROZEN_DIR):
        if os.path.exists(PYTHON_BACKUP_DIR):
            shutil.rmtree(PYTHON_BACKUP_DIR)
        shutil.move(PYTHON_FROZEN_DIR, PYTHON_BACKUP_DIR)
    else:
        raise ExtensionError(
            "No frozen RST found! Build OpenColorIO with CMake option "
            "'-DOCIO_BUILD_FROZEN_DOCS=ON' to generate required frozen "
            "documentation source files in: {dir}".format(dir=PYTHON_FROZEN_DIR)
        )

    logger.info("Backup complete.")


def compare_frozen(app, exception):
    """
    Compare backed up frozen RST files from ``backup_frozen()`` with 
    newly frozen RST. If they don't match within accepted tolerances, 
    raise an exception to fail the current CI job. This implies that 
    the OCIO API changed without building with CMake option 
    ``-DOCIO_BUILD_FROZEN_DOCS=ON``; needed to update frozen RST in the 
    source tree.
    """
    # Raise Sphinx exceptions for debugging
    if exception:
        raise ExtensionError(str(exception))

    if not app.config.frozendoc_compare:
        return

    logger.info("Comparing frozen RST: {src} <-> {dst}\n".format(
        src=PYTHON_FROZEN_DIR, 
        dst=PYTHON_BACKUP_DIR
    ))

    frozen_files = set(os.listdir(PYTHON_FROZEN_DIR))
    backup_files = set(os.listdir(PYTHON_BACKUP_DIR))

    # Find any files which are different, or only present in one of the two 
    # directories.
    match, mismatch, errors = filecmp.cmpfiles(
        PYTHON_FROZEN_DIR, 
        PYTHON_BACKUP_DIR,
        list(frozen_files | backup_files),
        shallow=False
    )

    # Different OSs or compilers may result in slightly different signatures
    # or types. Test each mismatched file for the ratio of how different 
    # their contents are. If they have a difference ratio at or above 0.6 (the 
    # recommended ratio to determine close matches), assume the differences are
    # ok. This is certainly not a water tight assumption, but deemed (at the 
    # moment) preferrable to failing on every mior difference. Keep track of 
    # these ignored differences and report them for transparency in CI logs.
    ignored = []

    for i in reversed(range(len(mismatch))):
        filename = mismatch[i]
        logger.info("Difference found in: {}".format(filename))

        frozen_path = os.path.join(PYTHON_FROZEN_DIR, filename)
        with open(frozen_path, "r") as frozen_file:
            frozen_data = frozen_file.read()

        backup_path = os.path.join(PYTHON_BACKUP_DIR, filename)
        with open(backup_path, "r") as backup_file:
            backup_data = backup_file.read()

        for line in difflib.unified_diff(
            frozen_data.splitlines(), 
            backup_data.splitlines(), 
            fromfile=frozen_path, 
            tofile=backup_path
        ):
            logger.info(line)

        # Based on difflib's caution of argument order playing a role in the 
        # results of ratio(), check the ratio in both directions and use the
        # better of the two as our heuristic.
        match_ab = difflib.SequenceMatcher(None, frozen_data, backup_data)
        match_ba = difflib.SequenceMatcher(None, backup_data, frozen_data)
        max_ratio = max(match_ab.ratio(), match_ba.ratio())

        if max_ratio >= 0.6:
            ignored.append(mismatch.pop(i))
            logger.info(
                "Difference ratio {} is within error tolerances\n".format(
                    max_ratio
                )
            )
        else:
            logger.error(
                "Difference ratio {} exceeds error tolerances\n".format(
                    max_ratio
                )
            )

    if os.path.exists(PYTHON_BACKUP_DIR):
        shutil.rmtree(PYTHON_BACKUP_DIR)

    if mismatch or errors:
        raise ExtensionError(
            "Frozen RST is out of date! Build OpenColorIO with CMake option "
            "'-DOCIO_BUILD_FROZEN_DOCS=ON' to update required frozen "
            "documentation source files in: {dir}\n\n"
            "    Changed files: {changed}\n\n"
            "      Added files: {added}\n\n"
            "    Removed files: {removed}\n\n"
            "See log for changed file differences.\n".format(
                dir=PYTHON_FROZEN_DIR,
                changed=", ".join(mismatch),
                added=", ".join(f for f in errors if f in frozen_files),
                removed=", ".join(f for f in errors if f in backup_files),
            )
        )

    # Report ignored differences
    if ignored:
        logger.warning(
            "Some differences were found and ignored when comparing frozen "
            "RST to current documentation source in: {dir}\n\n"
            "    Changed files: {changed}\n\n"
            "See log for changed file differences.\n".format(
                dir=PYTHON_FROZEN_DIR,
                changed=", ".join(ignored),
            )
        )

    logger.info("Frozen RST matches within error tolerances.")


def setup(app):
    app.add_config_value("frozendoc_build", False, "env")
    app.add_config_value("frozendoc_compare", False, "env")
    app.connect("builder-inited", backup_frozen)
    app.connect("build-finished", compare_frozen)
