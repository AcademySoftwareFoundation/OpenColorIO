#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

YAMLCPP_VERSION="$1"
INSTALL_TARGET="$2"

YAMLCPP_MAJOR_MINOR=$(echo "${YAMLCPP_VERSION}" | cut -d. -f-2)
YAMLCPP_MINOR=$(echo "${YAMLCPP_MAJOR_MINOR}" | cut -d. -f2-)
YAMLCPP_PATCH=$(echo "${YAMLCPP_VERSION}" | cut -d. -f3-)

# Required for CMake-4.0+ compatibility
export CMAKE_POLICY_VERSION_MINIMUM=3.5

git clone https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp

if [ "$YAMLCPP_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    if [[ "$YAMLCPP_MINOR" -lt 6 && "$YAMLCPP_PATCH" -lt 3 ]]; then
        git checkout tags/release-${YAMLCPP_VERSION} -b release-${YAMLCPP_VERSION}
    elif [[ "$YAMLCPP_MINOR" -lt 8 ]]; then
        git checkout tags/yaml-cpp-${YAMLCPP_VERSION} -b yaml-cpp-${YAMLCPP_VERSION}
    else
        git checkout tags/${YAMLCPP_VERSION} -b ${YAMLCPP_VERSION}
    fi
fi

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      ${INSTALL_TARGET:+"-DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}""} \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      -DBUILD_SHARED_LIBS=ON \
      -DYAML_CPP_BUILD_TESTS=OFF \
      -DYAML_CPP_BUILD_TOOLS=OFF \
      -DYAML_CPP_BUILD_CONTRIB=OFF \
      ../.
cmake --build . \
      --target install \
      --config Release \
      --parallel 2

cd ../..
rm -rf yaml-cpp
