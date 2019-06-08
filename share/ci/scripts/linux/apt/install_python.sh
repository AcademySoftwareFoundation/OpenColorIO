#!/usr/bin/env bash

set -ex

PYTHON_VERSION="$1"

apt-get install -y python${PYTHON_VERSION}
apt-get install -y python${PYTHON_VERSION}-dev
