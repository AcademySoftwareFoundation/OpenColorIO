#!/usr/bin/env bash

set -ex

mkdir _coverage
cd _coverage

# The sed command below converts from:
#   ../_build/src/OpenColorIO/CMakeFiles/OpenColorIO.dir/ops/Exponent.gcno
# to:
#   ../src/OpenColorIO/ops/Exponent.cpp

for g in $(find ../_build -name "*.gcno" -type f); do
    gcov -p -o $(dirname "$g") $(echo "$g" | sed -e 's/\/_build\//\//' -e 's/\.gcno/\.cpp/' -e 's/\/CMakeFiles.*\.dir\//\//')
done

cd ..
