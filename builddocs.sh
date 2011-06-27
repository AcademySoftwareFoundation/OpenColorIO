#!/bin/sh

# Clone master branch into sub-directory
mkdir -p mastercopy
git archive master | tar -x -C mastercopy

# Run cmake in a build directory
mkdir -p build
cd build
cmake -D MAKE_DOCS=YES ../mastercopy

# Build the docs
make doc -j8

# Currently in build dir, move HTML up one level
cp -R docs/build-html/* ..

