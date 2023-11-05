#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

HERE=$(dirname $0)
DOXYGEN_LOCATION="$1"

bash $HERE/install_doxygen.sh "$DOXYGEN_LOCATION"
pip install -r $HERE/../../../../docs/requirements.txt
