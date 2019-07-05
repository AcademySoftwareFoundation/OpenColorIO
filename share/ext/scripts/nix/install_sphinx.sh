#!/usr/bin/env bash

set -ex

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    # Python 2
    SPHINX_VERSION="$1"
else
    # Python 3
    SPHINX_VERSION="$2"
fi

mkdir -p ext/dist

pip install --install-option="--prefix=${PWD}/ext/dist" -I Sphinx==${SPHINX_VERSION}
