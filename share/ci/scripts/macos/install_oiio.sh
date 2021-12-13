#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

OIIO_VERSION="$1"

if [ "$OIIO_VERSION" == "latest" ]; then
    brew install openimageio
else
    brew install openimageio@${OIIO_VERSION}
fi
