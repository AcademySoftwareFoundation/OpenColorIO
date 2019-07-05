#!/usr/bin/env bash

set -ex

SETUPTOOLS_VERSION="$1"

mkdir -p ext/dist

pip install --install-option="--prefix=${PWD}/ext/dist" -I setuptools==${SETUPTOOLS_VERSION}
