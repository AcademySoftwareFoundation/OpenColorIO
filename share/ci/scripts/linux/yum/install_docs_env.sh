#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

HERE=$(dirname $0)

# The yum install doxygen command started failing during CI since mirrorlist.centos.org
# was turned off. However, it seems that Doxygen is already installed on the containers
# used for CI, so this command is not currently necessary there. You will need to
# uncomment this line if you want to build the documentation on Linux but don't have
# doxygen already installed.
# bash $HERE/install_doxygen.sh latest

pip install -r $HERE/../../../../../docs/requirements.txt
