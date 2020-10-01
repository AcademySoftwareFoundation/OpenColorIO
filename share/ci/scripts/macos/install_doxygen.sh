#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

DOXYGEN_VERSION="$1"

if [ "DOXYGEN_VERSION" == "latest" ]; then
    sudo brew install doxygen
else
    sudo brew install doxygen@${DOXYGEN_VERSION}
fi
