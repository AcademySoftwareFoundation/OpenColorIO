#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

BOOST_VERSION="$1"

if [ "$BOOST_VERSION" == "latest" ]; then
    brew install boost
else
    brew install boost@${BOOST_VERSION}
fi
