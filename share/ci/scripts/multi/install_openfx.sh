#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

OPENFX_VERSION="$1"
INSTALL_TARGET="$2"

OPENFX_MAJOR_MINOR=$(echo "${OPENFX_VERSION}" | cut -d. -f-2)
OPENFX_MAJOR=$(echo "${OPENFX_VERSION}" | cut -d. -f-1)
OPENFX_MINOR=$(echo "${OPENFX_MAJOR_MINOR}" | cut -d. -f2-)
OPENFX_VERSION_U="${OPENFX_MAJOR}_${OPENFX_MINOR}"

git clone https://github.com/ofxa/openfx.git
cd openfx

if [ "$OPENFX_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/OFX_Release_${OPENFX_VERSION_U}_TAG -b OFX_Release_${OPENFX_VERSION_U}_TAG
fi

if [ -z "${INSTALL_TARGET}" ]; then
    sudo mkdir -p /usr/local/include/openfx
    sudo cp include/*.h /usr/local/include/openfx
else
    mkdir -p "${INSTALL_TARGET}"/include/openfx
    cp include/*.h "${INSTALL_TARGET}"/include/openfx
fi

cd ..
rm -rf openfx
