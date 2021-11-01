#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

BISON_VERSION="$1"

if [ "$BISON_VERSION" == "latest" ]; then
    brew install bison
else
    brew install bison@${BISON_VERSION}
fi
