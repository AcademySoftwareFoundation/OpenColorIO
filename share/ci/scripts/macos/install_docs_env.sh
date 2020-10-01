#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

bash install_doxygen.sh latest
bash ../linux/install_sphinx.sh latest
bash ../linux/install_testresources.sh latest
bash ../linux/install_recommonmark.sh latest
bash ../linux/install_sphinx-press-theme.sh latest
bash ../linux/install_breathe.sh latest
