#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    # Python 2
    SPHINX_PRESS_THEME_VERSION="$1"
else
    # Python 3
    SPHINX_PRESS_THEME_VERSION="${2:-${1}}"
fi

if [ "$SPHINX_PRESS_THEME_VERSION" == "latest" ]; then
    sudo pip install sphinx-press-theme
else
    sudo pip install sphinx-press-theme==${SPHINX_PRESS_THEME_VERSION}
fi
