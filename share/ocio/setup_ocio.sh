#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

OCIO_ROOT="/Users/frankeder/Documents/ocio2/OpenColorIO"
OCIO_EXECROOT="/Users/frankeder/Documents/ocio2/built"

export OCIO="/Users/frankeder/Documents/ocio2/aces_1.0.3/config.ocio"


# For OS X
export DYLD_LIBRARY_PATH="${OCIO_EXECROOT}/lib:${DYLD_LIBRARY_PATH}"

# For Linux
#export LD_LIBRARY_PATH="${OCIO_EXECROOT}/lib:${LD_LIBRARY_PATH}"

export PATH="${OCIO_EXECROOT}/bin:${PATH}"
export PATH="${OCIO_EXECROOT}/lib/python2.7:${PATH}"
export PYTHONPATH="${OCIO_EXECROOT}/lib/python2.7:${PYTHONPATH}"
export NUKE_PATH="${OCIO_EXECROOT}/lib/nuke@Nuke_API_VERSION@:${NUKE_PATH}"
export NUKE_PATH="${OCIO_ROOT}/share/nuke:${NUKE_PATH}";
