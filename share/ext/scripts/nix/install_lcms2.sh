#!/usr/bin/env bash

set -ex

LCMS2_VERSION="$1"

mkdir -p ext/dist
mkdir -p ext/tmp
cd ext/tmp

git clone https://github.com/mm2/Little-CMS.git
cd Little-CMS

if [ "$LCMS2_VERSION" != "latest" ]; then
    git checkout tags/lcms${LCMS2_VERSION} -b lcms${LCMS2_VERSION}
fi

cat >CMakeLists.txt << 'EOF'
project(lcms2)

cmake_minimum_required(VERSION 2.8)

include_directories(include)

file(GLOB HEADERS "include/*.h")
file(GLOB SOURCES "src/*.c" "src/*.h")

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_NAME "${PROJECT_NAME}"
    PUBLIC_HEADER "${HEADERS}"
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/lcms2
)
EOF

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=../../../dist \
      -DCMAKE_C_FLAGS="-fPIC" \
      ../.
make -j4
make install

cd ../../..
rm -rf tmp
