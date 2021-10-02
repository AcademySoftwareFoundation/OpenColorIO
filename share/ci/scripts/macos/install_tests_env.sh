#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

HERE=$(dirname $0)

pip install -r $HERE/../../../../tests/python/requirements.txt
