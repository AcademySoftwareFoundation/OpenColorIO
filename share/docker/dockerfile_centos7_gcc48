#
# Centos 7 Dockerfile for OCIO
#

# Pull base image
FROM centos:7

LABEL maintainer="sean@seancooper.xyz"

# Install yum hosted dependencies
RUN yum -y install epel-release && \
    yum -y install gcc-c++ \
    make \
    cmake3 \
    git \
    wget \
    bzip2 \
    file \
    which \
    patch \
    python-devel.x86_64 \
    python-sphinx \
    glew-devel \
    java \
    libtiff-devel \
    zlib \
    libpng-devel \
    libjpeg-turbo-devel \
    bzip2-devel \
    freeglut-devel \
    libXmu-devel \
    libXi-devel && \
    yum -y clean all
    
# Install Truelight library
RUN mkdir /home/download && \
    cd /home/download && \
    wget -O truelight-4.0.7863_64.run --post-data 'access=public&download=truelight/4_0/truelight-4.0.7863_64.run&last_page=/support/customer-login/truelight_sp/truelight_40.php' https://www.filmlight.ltd.uk/resources/download.php && \
    sh truelight-4.0.7863_64.run

# Install Boost
RUN cd /home/download && \
    wget https://sourceforge.net/projects/boost/files/boost/1.61.0/boost_1_61_0.tar.gz && \
    tar -xzf boost_1_61_0.tar.gz && \
    cd boost_1_61_0 && \
    sh bootstrap.sh && \
    ./b2 install --prefix=/usr/local

# Install OpenEXR
RUN mkdir /home/git && \
    cd /home/git && \
    git clone https://github.com/openexr/openexr.git -b v2.2.1 && \
    cd /home/git/openexr/IlmBase && \
    mkdir /home/git/openexr/IlmBase/build && \
    cd /home/git/openexr/IlmBase/build && \
    cmake3 .. && \
    make install -j2 && \
    cd /home/git/openexr/OpenEXR && \
    mkdir /home/git/openexr/OpenEXR/build && \
    cd /home/git/openexr/OpenEXR/build && \
    cmake3 -DILMBASE_PACKAGE_PREFIX=/usr/local .. && \
    make install -j2

# Install OIIO
RUN cd /home/git && \
    git clone https://github.com/OpenImageIO/oiio.git -b RB-1.7 && \
    cd /home/git/oiio && \
    mkdir build && \
    cd /home/git/oiio/build && \
    cmake3 .. && \
    make install -j2

# Remove cache
RUN rm -rf /var/cache/yum && \
    rm -R /home/git && \
    rm -R /home/download
