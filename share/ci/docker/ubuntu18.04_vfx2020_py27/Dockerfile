FROM ubuntu:18.04

COPY share/ci/scripts/* /tmp/

# TODO: Change to Python 3.7 when OCIO supports it

RUN /tmp/install.sh 6 6.0 2.7
RUN /tmp/install_numpy.sh 1.16
RUN /tmp/install_cmake.sh 3 14 4
RUN /tmp/install_truelight.sh 4 0 7863
RUN /tmp/install_boost.sh 1 70 0
RUN /tmp/install_openexr.sh 2.3.0
RUN /tmp/install_oiio.sh 2.0.8
RUN /tmp/install_sonar.sh 3.3.0.1492
