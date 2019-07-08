#!/usr/bin/env bash

set -ex

SOURCE_DIR="${1:-.}"

# Notes:
# - Dependencies are installed to ${PWD}/ext/dist
# - C/C++ dependencies are built as static libraries
# - Python packages are installed for the system default python version
# - Installed packages will be found by the OCIO build automatically

# Python dependencies:
# SCRIPT                                               PY2 VER  PY3 VER
${SOURCE_DIR}/share/ext/scripts/install_setuptools.sh  1.1.6    1.1.6
${SOURCE_DIR}/share/ext/scripts/install_docutils.sh    0.14     0.13
${SOURCE_DIR}/share/ext/scripts/install_markupsafe.sh  1.1.1    1.1.1
${SOURCE_DIR}/share/ext/scripts/install_jinja2.sh      2.10.1   2.10.1
${SOURCE_DIR}/share/ext/scripts/install_pygments.sh    2.4.2    2.4.2
${SOURCE_DIR}/share/ext/scripts/install_sphinx.sh      1.8.5    2.1.2

# C++ dependencies:
# SCRIPT                                             VER
${SOURCE_DIR}/share/ext/scripts/install_expat.sh     2.2.7
${SOURCE_DIR}/share/ext/scripts/install_half.sh      2.3.0
${SOURCE_DIR}/share/ext/scripts/install_yaml-cpp.sh  0.3.0
${SOURCE_DIR}/share/ext/scripts/install_lcms2.sh     2.2
