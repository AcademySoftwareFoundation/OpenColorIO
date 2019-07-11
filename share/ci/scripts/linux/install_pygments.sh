#!/usr/bin/env bash

set -ex

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    # Python 2
    PYGMENTS_VERSION="$1"
else
    # Python 3
    PYGMENTS_VERSION="$2"
fi

pip install Pygments==${PYGMENTS_VERSION}
