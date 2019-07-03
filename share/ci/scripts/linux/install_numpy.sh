#!/usr/bin/env bash

set -ex

NUMPY_VERSION="$1"

pip install numpy>=${NUMPY_VERSION}
