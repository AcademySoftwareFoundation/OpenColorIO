#!/usr/bin/env bash

set -ex

GCC_VERSION="$1"
CLANG_VERSION="$2"
PYTHON_VERSION="$3"

apt-get update
apt-get -y upgrade

apt-get install -y git
apt-get install -y wget
apt-get install -y unzip
apt-get install -y g++-${GCC_VERSION}
apt-get install -y clang-${CLANG_VERSION}
apt-get install -y python${PYTHON_VERSION}
apt-get install -y python${PYTHON_VERSION}-dev
apt-get install -y python-pip
apt-get install -y libgl1-mesa-dev
apt-get install -y freeglut3-dev
apt-get install -y libglew-dev
apt-get install -y libxmu-dev
apt-get install -y libxi-dev
apt-get install -y zlib1g-dev
apt-get install -y bzip2
apt-get install -y libbz2-dev
apt-get install -y libjpeg-turbo8-dev
apt-get install -y libpng-dev
apt-get install -y libtiff-dev

update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 100 \
                    --slave /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION}

update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${CLANG_VERSION} 100 \
                    --slave /usr/bin/clang++ clang++ /usr/bin/clang++-${CLANG_VERSION}

export CC="/usr/bin/gcc"
export CXX="/usr/bin/g++"

apt-get clean
rm -rf /var/lib/apt/lists/*
