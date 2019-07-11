#!/usr/bin/env bash

set -ex

DOCUTILS_PY2_VERSION="$1"
DOCUTILS_PY3_VERSION="$2"

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    pip install docutils==${DOCUTILS_PY2_VERSION}
else
    pip install docutils-python3==${DOCUTILS_PY3_VERSION}
fi
