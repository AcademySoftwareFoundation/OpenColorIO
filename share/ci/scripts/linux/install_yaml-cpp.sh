#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

YAMLCPP_VERSION="$1"
YAMLCPP_MAJOR_MINOR=$(echo "${YAMLCPP_VERSION}" | cut -d. -f-2)
YAMLCPP_MINOR=$(echo "${YAMLCPP_MAJOR_MINOR}" | cut -d. -f2-)
YAMLCPP_PATCH=$(echo "${YAMLCPP_VERSION}" | cut -d. -f3-)

git clone https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp

if [ "$YAMLCPP_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    if [[ "$YAMLCPP_MINOR" -lt 6 && "$YAMLCPP_PATCH" -lt 3 ]]; then
        git checkout tags/release-${YAMLCPP_VERSION} -b release-${YAMLCPP_VERSION}
    else
        git checkout tags/yaml-cpp-${YAMLCPP_VERSION} -b yaml-cpp-${YAMLCPP_VERSION}
    fi
fi

mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=ON \
      -DYAML_CPP_BUILD_TESTS=OFF \
      -DYAML_CPP_BUILD_TOOLS=OFF \
      -DYAML_CPP_BUILD_CONTRIB=OFF \
      -DCMAKE_CXX_FLAGS="-fPIC \
                         -Wno-deprecated-declarations" \
      ../.
make -j4
sudo make install

cd ../..
rm -rf yaml-cpp
