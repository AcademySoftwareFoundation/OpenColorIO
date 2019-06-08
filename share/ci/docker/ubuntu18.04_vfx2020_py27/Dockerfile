FROM ubuntu:18.04

COPY share/ci/scripts/* /tmp/

# TODO: Change to Python 3.7 when OCIO supports it

RUN /tmp/install.sh && \
    /tmp/install_gcc.sh 6 && \
    /tmp/install_clang.sh 6.0 && \
    /tmp/install_python.sh 2.7 && \
    /tmp/install_numpy.sh 1.16 && \
    /tmp/install_cmake.sh 3.14.4 && \
    /tmp/install_truelight.sh 4.0.7863 && \
    /tmp/install_boost.sh 1.70.0 && \
    /tmp/install_openexr.sh 2.3.0 && \
    /tmp/install_oiio.sh 2.0.8 && \
    /tmp/install_sonar.sh 3.3.0.1492 && \
    /tmp/clean.sh
