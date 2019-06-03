FROM ubuntu:18.04

COPY share/ci/scripts/* /tmp/

RUN /tmp/install.sh 6 6.0 2.7 && \
    /tmp/install_numpy.sh 1.14 && \
    /tmp/install_cmake.sh 3 14 4 && \
    /tmp/install_truelight.sh 4 0 7863 && \
    /tmp/install_boost.sh 1 66 0 && \
    /tmp/install_openexr.sh 2.3.0 && \
    /tmp/install_oiio.sh 2.0.8 && \
    /tmp/install_sonar.sh 3.3.0.1492

