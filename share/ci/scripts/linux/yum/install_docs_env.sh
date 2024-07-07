#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

HERE=$(dirname $0)

# The yum install doxygen command started failing since mirrorlist.centos.org
# was turned off. However, it seems that Doxygen is already installed on our containers
# so this command is not currently necessary:
# bash $HERE/install_doxygen.sh latest

pip install -r $HERE/../../../../../docs/requirements.txt
