#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# The aswf/ci-ocio container images intentionally do not ship OpenColorIO,
# since the whole point of the image is to build OpenColorIO from source.
# However, the container's prebuilt OpenImageIO is dynamically linked
# against the specific OpenColorIO release used by the corresponding
# aswf-docker VFX Reference Platform year, and that library is not present
# in the image. Fetch the matching prebuilt OpenColorIO shared library from
# the ASWF Conan remote and stage it on the linker search path, so apps
# built against OCIO_USE_OIIO_FOR_APPS=ON can resolve OpenImageIO's
# transitive dependency at link and run time.
#
# Usage: install_aswf_opencolorio.sh <opencolorio-version> <vfx-year>

set -ex

OCIO_VERSION="$1"
VFX_YEAR="$2"
CONAN_REF="opencolorio/${OCIO_VERSION}@aswf/vfx${VFX_YEAR}"

conan remote add aswf https://linuxfoundation.jfrog.io/artifactory/api/conan/aswf-conan

PKG_ID=$(conan list "${CONAN_REF}:*" -r aswf --format=json 2>/dev/null \
    | jq -r --arg ref "${CONAN_REF}" '.aswf[$ref].revisions | to_entries[0].value.packages | keys[0]')

conan download "${CONAN_REF}:${PKG_ID}" -r aswf

PKG_PATH=$(conan cache path "${CONAN_REF}:${PKG_ID}")

cp -P "${PKG_PATH}"/lib/libOpenColorIO.so* /usr/local/lib/
ldconfig
