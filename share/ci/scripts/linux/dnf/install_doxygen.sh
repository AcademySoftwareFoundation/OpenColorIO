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
else
    # Is some version of doxygen already available?
    if command -v doxygen >/dev/null; then
        if [ "$DOXYGEN_VERSION" == "latest" ]; then
            # Get latest doxygen version from GH
            DOXYGEN_VERSION=$(
                curl --silent https://api.github.com/repos/doxygen/doxygen/releases/latest | \
                grep '"tag_name":' | \
                sed -E 's/.*"Release_([0-9_]+)".*/\1/' | \
                tr '_' '.'
            )
        fi

        # Manual install
        wget https://doxygen.nl/files/doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz
        tar -xzf doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz
        cp doxygen-${DOXYGEN_VERSION}/bin/doxygen /usr/local/bin/
    fi
fi
