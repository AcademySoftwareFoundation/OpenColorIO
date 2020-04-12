#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

PYTHON_VERSION="$1"
PYTHON_MAJOR=$(echo "${PYTHON_VERSION}" | cut -d. -f-1)

wget https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz
tar xzf Python-${PYTHON_VERSION}.tgz

cd Python-${PYTHON_VERSION}
./configure
make -j4
sudo make install

if [[ "$PYTHON_MAJOR" == "3" ]]; then
    sudo ln -sf /usr/local/bin/python3 /usr/local/bin/python
fi

cd ..
rm -rf Python-${PYTHON_VERSION}

wget https://bootstrap.pypa.io/get-pip.py
sudo python get-pip.py
rm get-pip.py
# This step is needed to make this pip install functional alongside a Python 
# 2.7 pip install that may already be present.
python -m pip install --upgrade pip

if [[ "$PYTHON_MAJOR" == "3" ]]; then
    sudo ln -sf /usr/local/bin/pip3 /usr/local/bin/pip
fi
