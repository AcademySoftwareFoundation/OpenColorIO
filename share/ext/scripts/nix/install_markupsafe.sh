#!/usr/bin/env bash

set -ex

MARKUPSAFE_VERSION="$1"

mkdir -p ext/dist

pip install --install-option="--prefix=${PWD}/ext/dist" -I MarkupSafe==${MARKUPSAFE_VERSION}
