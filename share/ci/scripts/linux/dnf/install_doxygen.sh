#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

DOXYGEN_VERSION="$1"

DOXYGEN_MAJOR_MINOR=$(echo "${DOXYGEN_VERSION}" | cut -d. -f-2)
DOXYGEN_MAJOR=$(echo "${DOXYGEN_VERSION}" | cut -d. -f-1)
DOXYGEN_MINOR=$(echo "${DOXYGEN_MAJOR_MINOR}" | cut -d. -f2-)
DOXYGEN_PATCH=$(echo "${DOXYGEN_VERSION}" | cut -d. -f3-)
DOXYGEN_VERSION_U="${DOXYGEN_MAJOR}_${DOXYGEN_MINOR}_${DOXYGEN_PATCH}"

if command -v dnf >/dev/null; then
    if [ "$DOXYGEN_VERSION" == "latest" ]; then
        dnf install -y doxygen
    else
        dnf install -y doxygen-${DOXYGEN_VERSION}
    fi
else
    # Is some version of doxygen already available?
    if command -v doxygen >/dev/null; then
        source /etc/os-release
        if [ "$ID" = "centos" ]; then
            # CentOS-7 was known to be compatible with this doxygen version
            DOXYGEN_VERSION=1.9.0
            DOXYGEN_VERSION_U=1_9_0

        elif [ "$DOXYGEN_VERSION" = "latest" ]; then
            # Get latest doxygen version from GH
            DOXYGEN_VERSION_U=$(
                curl --silent https://api.github.com/repos/doxygen/doxygen/releases/latest | \
                grep '"tag_name":' | \
                sed -E 's/.*"Release_([0-9_]+)".*/\1/'
            )
            DOXYGEN_VERSION=$(tr $DOXYGEN_VERSION_U '_' '.')
        fi

        # Manual install
        wget https://github.com/doxygen/doxygen/releases/download/Release_${DOXYGEN_VERSION_U}/doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz
        tar -xzf doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz
        cp doxygen-${DOXYGEN_VERSION}/bin/doxygen /usr/local/bin/
    fi
fi
