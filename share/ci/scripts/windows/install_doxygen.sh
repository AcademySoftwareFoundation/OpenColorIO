#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

DOXYGEN_VERSION="$1"

if [ "$DOXYGEN_VERSION" == "latest" ]; then
    choco install doxygen.install
else
    choco install doxygen.install --version=${DOXYGEN_VERSION}
fi
