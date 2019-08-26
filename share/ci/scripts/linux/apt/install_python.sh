#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

PYTHON_VERSION="$1"

apt-get install -y python${PYTHON_VERSION}
apt-get install -y python${PYTHON_VERSION}-dev
