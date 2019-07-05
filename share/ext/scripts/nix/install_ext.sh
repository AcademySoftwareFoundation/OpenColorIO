#!/usr/bin/env bash

set -ex

SOURCE_DIR="${1:-.}"

# Python dependencies:
#   - <script>.sh <ver>
#   - <script>.sh <py2 ver> <py3 ver>
${SOURCE_DIR}/share/ext/scripts/nix/install_setuptools.sh 1.1.6
${SOURCE_DIR}/share/ext/scripts/nix/install_docutils.sh 0.14 0.13
${SOURCE_DIR}/share/ext/scripts/nix/install_markupsafe.sh 1.1.1
${SOURCE_DIR}/share/ext/scripts/nix/install_jinja2.sh 2.10.1
${SOURCE_DIR}/share/ext/scripts/nix/install_pygments.sh 2.4.2
${SOURCE_DIR}/share/ext/scripts/nix/install_sphinx.sh 1.8.5 2.1.2

# C++ dependencies:
#   - <script>.sh <ver>
${SOURCE_DIR}/share/ext/scripts/nix/install_expat.sh 2.2.7
${SOURCE_DIR}/share/ext/scripts/nix/install_half.sh 2.3.0
${SOURCE_DIR}/share/ext/scripts/nix/install_yaml-cpp.sh 0.3.0
${SOURCE_DIR}/share/ext/scripts/nix/install_lcms2.sh 2.2
