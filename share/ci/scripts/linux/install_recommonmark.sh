#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    # Python 2
    RECOMMONMARK_VERSION="$1"
else
    # Python 3
    RECOMMONMARK_VERSION="${2:-${1}}"
fi

if [ "$RECOMMONMARK_VERSION" == "latest" ]; then
    sudo pip install recommonmark
else
    sudo pip install recommonmark==${RECOMMONMARK_VERSION}
fi
