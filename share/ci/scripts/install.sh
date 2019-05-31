#!/usr/bin/env bash

set -ex

GCC_VERSION="$1"
CLANG_VERSION="$2"
PYTHON_VERSION="$3"

apt-get update
apt-get -y upgrade

RUN apt-get -y git
RUN apt-get -y wget
RUN apt-get -y unzip
RUN apt-get -y g++-$(GCC_VERSION)
RUN apt-get -y clang-$(CLANG_VERSION)
RUN apt-get -y python${PYTHON_VERSION}
RUN apt-get -y python${PYTHON_VERSION}-dev
RUN apt-get -y python-pip
RUN apt-get -y libgl1-mesa-dev
RUN apt-get -y freeglut3-dev
RUN apt-get -y libglew-dev
RUN apt-get -y libxmu-dev
RUN apt-get -y libxi-dev
RUN apt-get -y zlib1g-dev
RUN apt-get -y bzip2
RUN apt-get -y libbz2-dev
RUN apt-get -y libjpeg-turbo8-dev
RUN apt-get -y libpng-dev
RUN apt-get -y libtiff-dev

update-alternatives --remove-all gcc
update-alternatives --remove-all g++

update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 100 \
                    --slave /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION}

update-alternatives --remove-all clang
update-alternatives --remove-all clang++

update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${CLANG_VERSION} 100 \
                    --slave /usr/bin/clang++ clang++ /usr/bin/clang++-${CLANG_VERSION}

export CC="/usr/bin/gcc"
export CXX="/usr/bin/g++"

apt-get clean
rm -rf /var/lib/apt/lists/*
