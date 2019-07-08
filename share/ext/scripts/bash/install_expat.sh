#!/usr/bin/env bash

set -ex

EXPAT_VERSION="$1"
EXPAT_MAJOR_MINOR=$(echo "${EXPAT_VERSION}" | cut -d. -f-2)
EXPAT_MAJOR=$(echo "${EXPAT_VERSION}" | cut -d. -f-1)
EXPAT_MINOR=$(echo "${EXPAT_MAJOR_MINOR}" | cut -d. -f2-)
EXPAT_PATCH=$(echo "${EXPAT_VERSION}" | cut -d. -f3-)
EXPAT_VERSION_U="${EXPAT_MAJOR}_${EXPAT_MINOR}_${EXPAT_PATCH}"

mkdir -p ext/dist
mkdir -p ext/tmp
cd ext/tmp

git clone https://github.com/libexpat/libexpat.git
cd libexpat

if [ "$EXPAT_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/R_${EXPAT_VERSION_U} -b R_${EXPAT_VERSION_U}
fi

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=../../../dist \
      -DBUILD_examples=OFF \
      -DBUILD_tests=OFF \
      -DBUILD_shared=OFF \
      -DCMAKE_C_FLAGS="-fPIC" \
      -DCMAKE_CXX_FLAGS="-fPIC" \
      ../expat/.
make -j4
make install

cd ../../..
rm -rf tmp

mkdir -p dist/include/expat
mv dist/include/expat*.h dist/include/expat/.
