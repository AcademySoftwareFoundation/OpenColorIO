FROM ubuntu:18.04

COPY share/ci/scripts/* /tmp/

RUN /tmp/linux/apt/install.sh && \
    /tmp/linux/apt/install_gcc.sh 6 && \
    /tmp/linux/apt/install_clang.sh 7 && \
    /tmp/linux/apt/install_python.sh 2.7 && \
    /tmp/linux/install_numpy.sh 1.14 && \
    /tmp/linux/install_cmake.sh 3.11.4 && \
    /tmp/linux/install_boost.sh 1.66.0 && \
    /tmp/linux/install_openexr.sh 2.3.0 && \
    /tmp/linux/install_oiio.sh 2.0.8 && \
    /tmp/linux/install_sonar.sh 3.3.0.1492 && \
    /tmp/linux/apt/clean.sh
