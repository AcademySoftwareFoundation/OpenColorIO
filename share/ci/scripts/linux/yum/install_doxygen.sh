#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

DOXYGEN_VERSION="$1"
YUM_CMD="yum"

# RockyLinux instead of CentOS?
if command -v dnf >/dev/null; then
    YUM_CMD="dnf"
fi

$YUM_CMD install -y epel-release
if [ "$DOXYGEN_VERSION" == "latest" ]; then
    $YUM_CMD install -y doxygen
else
    $YUM_CMD install -y doxygen-${DOXYGEN_VERSION}
fi
