#!/usr/bin/env bash

set -ex

PYGMENTS_VERSION="$1"

mkdir -p ext/dist

pip install --install-option="--prefix=${PWD}/ext/dist" -I Pygments==${PYGMENTS_VERSION}
