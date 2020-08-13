#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    # Python 2
    TESTRESOURCES_VERSION="$1"
else
    # Python 3
    TESTRESOURCES_VERSION="${2:-${1}}"
fi

if [ "$TESTRESOURCES_VERSION" == "latest" ]; then
    sudo pip install testresources
else
    sudo pip install testresources==${TESTRESOURCES_VERSION}
fi
