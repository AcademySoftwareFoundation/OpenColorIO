#!/usr/bin/env bash

set -ex

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    # Python 2
    NUMPY_VERSION="$1"
else
    # Python 3
    NUMPY_VERSION="$2"
fi

sudo pip install numpy==${NUMPY_VERSION}
