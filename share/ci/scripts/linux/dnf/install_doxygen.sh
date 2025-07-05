#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

DOXYGEN_VERSION="$1"

if command -v dnf >/dev/null; then
    if [ "$DOXYGEN_VERSION" == "latest" ]; then
        dnf install -y doxygen
    else
        dnf install -y doxygen-${DOXYGEN_VERSION}
    fi
elif command -v doxygen >/dev/null; then
    # Get any working version...
    yum install -y doxygen
fi
