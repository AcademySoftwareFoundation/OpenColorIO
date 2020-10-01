#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    # Python 2
    BREATHE_VERSION="$1"
else
    # Python 3
    BREATHE_VERSION="${2:-${1}}"
fi

if [ "$BREATHE_VERSION" == "latest" ]; then
    pip install breathe
else
    pip install breathe==${BREATHE_VERSION}
fi
