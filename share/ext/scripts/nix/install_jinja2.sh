#!/usr/bin/env bash

set -ex

JINJA2_VERSION="$1"

mkdir -p ext/dist

pip install --install-option="--prefix=${PWD}/ext/dist" -I Jinja2==${JINJA2_VERSION}
