#!/usr/bin/env bash

set -ex

BOOST_MAJOR="$1"
BOOST_MINOR="$2"
BOOST_PATCH="$3"
BOOST_MAJOR_MINOR="${BOOST_MAJOR_VER}.${BOOST_MINOR_VER}"
BOOST_VERSION="${BOOST_MAJOR_VER}.${BOOST_MINOR_VER}.${BOOST_PATCH_VER}"
BOOST_VERSION_U="${BOOST_MAJOR_VER}_${BOOST_MINOR_VER}_${BOOST_PATCH_VER}"

echo "using python : : /usr/bin/python${PYTHON_VERSION} ;\n" \
     "using gcc : : ${CXX} ;\n" \
     > ${HOME}/user-config.jam

mkdir _boost
cd _boost

wget -q https://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/boost_${BOOST_VERSION_U}.tar.gz
tar -xzf boost_${BOOST_VERSION_U}.tar.gz

cd boost_${BOOST_VERSION_U}
sh bootstrap.sh
./b2 install -j4 variant=release toolset=gcc \
    --with-system \
    --with-regex \
    --with-filesystem \
    --with-thread \
    --with-python \
    --prefix=/usr/local

cd ../..
rm -rf _boost
