#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

mkdir _coverage
cd _coverage

# The sed command below converts from:
#   ../_build/src/OpenColorIO/CMakeFiles/OpenColorIO.dir/ops/Exponent.gcno
# to:
#   ../src/OpenColorIO/ops/Exponent.cpp

for g in $(find ../_build -name "*.gcno" -type f); do
    gcov -l -p -o $(dirname "$g") $(echo "$g" | sed -e 's/\/_build\//\//' -e 's/\.gcno/\.cpp/' -e 's/\/CMakeFiles.*\.dir\//\//')
done

cd ..
