#!/usr/bin/env bash

set -ex

CLANG_VERSION="$1"

apt-get install -y clang-${CLANG_VERSION}

update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${CLANG_VERSION} 100 \
                    --slave /usr/bin/clang++ clang++ /usr/bin/clang++-${CLANG_VERSION}
