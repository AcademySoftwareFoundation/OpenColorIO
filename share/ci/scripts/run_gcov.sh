#!/usr/bin/env bash

set -ex

mkdir _coverage
cd _coverage

for d in $(find ../src/ -name "*.dir" -type d); do
    gcov "$(dirname $(dirname "$d"))/*.cpp" --object-directory "$d"
done

cd ..
