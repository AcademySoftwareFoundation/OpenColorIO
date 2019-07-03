#!/usr/bin/env bash

set -ex

GCC_VERSION="$1"

apt-get install -y g++-${GCC_VERSION}

update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 100 \
                    --slave /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION}
