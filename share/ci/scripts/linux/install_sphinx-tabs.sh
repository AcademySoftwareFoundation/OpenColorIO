#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    # Python 2
    SPHINX_TABS_VERSION="$1"
else
    # Python 3
    SPHINX_TABS_VERSION="${2:-${1}}"
fi

if [ "$SPHINX_TABS_VERSION" == "latest" ]; then
    sudo pip install sphinx-tabs
else
    sudo pip install sphinx-tabs==${SPHINX_TABS_VERSION}
fi
