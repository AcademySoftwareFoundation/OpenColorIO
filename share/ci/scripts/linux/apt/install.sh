#!/usr/bin/env bash

set -ex

apt-get update
apt-get -y upgrade

apt-get install -y git
apt-get install -y wget
apt-get install -y unzip
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
