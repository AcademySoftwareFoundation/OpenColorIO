#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    # Python 2
    NUMPY_VERSION="$1"
else
    # Python 3
    NUMPY_VERSION="${2:-${1}}"
fi

if [ "$NUMPY_VERSION" == "latest" ]; then
    sudo pip3 install numpy
else
    sudo pip3 install numpy==${NUMPY_VERSION}
fi