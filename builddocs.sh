#!/bin/sh

# Clone master branch into sub-directory
#mkdir -p mastercopy
#git archive master | tar -x -C mastercopy

# Run cmake in a build directory
#rm -rf build
mkdir -p build
cd build
cmake \
-D OCIO_BUILD_DOCS=YES \
-D OCIO_BUILD_STATIC=NO \
-D OCIO_BUILD_STATIC=NO \
-D OCIO_BUILD_APPS=NO \
-D OCIO_BUILD_TESTS=NO \
-D OCIO_BUILD_PYGLUE=NO \
/net/homedirs/jeremys/git/ocio.spi/

# Build the docs
make doc -j8

# Currently in build dir, move HTML up one level
cp -R docs/build-html/* ..

