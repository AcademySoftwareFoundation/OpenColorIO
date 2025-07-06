#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

DOXYGEN_VERSION="$1"

if ! command -v doxygen >/dev/null; then
    if command -v dnf >/dev/null; then
        if [ "$DOXYGEN_VERSION" == "latest" ]; then
            dnf install -y doxygen
        else
            dnf install -y doxygen-${DOXYGEN_VERSION}
        fi
    else
        source /etc/os-release
        if [ "$ID" = "centos" ] && [ "$VERSION_ID" = "7" ]; then
            mv /etc/yum.repos.d/CentOS-Base.repo /etc/yum.repos.d/CentOS-Base.repo.bak
            tee /etc/yum.repos.d/CentOS-Vault.repo > /dev/null <<EOF
[base]
name=CentOS-7 - Base
baseurl=http://vault.centos.org/7.9.2009/os/\$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

[updates]
name=CentOS-7 - Updates
baseurl=http://vault.centos.org/7.9.2009/updates/\$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

[extras]
name=CentOS-7 - Extras
baseurl=http://vault.centos.org/7.9.2009/extras/\$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7
EOF
            yum clean all
            yum makecache
        fi

        if [ "$DOXYGEN_VERSION" == "latest" ]; then
            yum install -y doxygen
        else
            yum install -y doxygen-${DOXYGEN_VERSION}
        fi
    fi
fi
