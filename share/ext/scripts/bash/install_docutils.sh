#!/usr/bin/env bash

set -ex

DOCUTILS_PY2_VERSION="$1"
DOCUTILS_PY3_VERSION="$2"

mkdir -p ext/dist

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    pip install --install-option="--prefix=${PWD}/ext/dist" -I docutils==${DOCUTILS_PY2_VERSION}
else
    pip install --install-option="--prefix=${PWD}/ext/dist" -I docutils-python3==${DOCUTILS_PY3_VERSION}
fi
