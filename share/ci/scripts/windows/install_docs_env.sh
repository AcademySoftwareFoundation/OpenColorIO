#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

HERE=$(dirname $0)

bash $HERE/install_doxygen.sh latest
bash $HERE/install_sphinx.sh latest
bash $HERE/install_testresources.sh latest
bash $HERE/install_recommonmark.sh latest
bash $HERE/install_sphinx-press-theme.sh latest
bash $HERE/install_breathe.sh latest
