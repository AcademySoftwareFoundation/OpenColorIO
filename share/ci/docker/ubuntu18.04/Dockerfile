# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

FROM ubuntu:18.04

COPY share/ci/scripts /tmp/share/ci/scripts/
COPY share/cmake /tmp/share/cmake/

WORKDIR /tmp

RUN share/ci/scripts/linux/apt/install.sh && \
    share/ci/scripts/linux/apt/install_gcc.sh 6 && \
    share/ci/scripts/linux/apt/install_clang.sh 7 && \
    share/ci/scripts/linux/apt/install_python.sh 2.7 && \
    share/ci/scripts/linux/install_numpy.sh 1.14 && \
    share/ci/scripts/linux/install_sphinx.sh 1.8.5 && \
    share/ci/scripts/linux/install_cmake.sh 3.12.4 && \
    share/ci/scripts/linux/install_expat.sh 2.2.5 && \
    share/ci/scripts/linux/install_lcms2.sh 2.2 && \
    share/ci/scripts/linux/install_yaml-cpp.sh 0.3.0 && \
    share/ci/scripts/linux/install_pystring.sh 1.1.3 && \
    share/ci/scripts/linux/install_boost.sh 1.66.0 && \
    share/ci/scripts/linux/install_openexr.sh 2.3.0 && \
    share/ci/scripts/linux/install_oiio.sh 2.0.8 && \
    share/ci/scripts/linux/install_sonar.sh 3.3.0.1492 && \
    share/ci/scripts/linux/apt/clean.sh
