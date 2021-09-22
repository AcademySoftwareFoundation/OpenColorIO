# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


if(OCIO_BUILD_NUKE)
    add_subdirectory(nuke)
endif()

if(OCIO_BUILD_OPENFX)
    add_subdirectory(openfx)
endif()
