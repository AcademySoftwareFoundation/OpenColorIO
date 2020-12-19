# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(TARGET test_gpu_exec OR
   TARGET ociodisplay OR
   TARGET ociochecklut OR
   TARGET ocioconvert
)
    add_subdirectory(oglapphelpers)
endif()

if(TARGET OpenColorIO::ImageIOBackend)
    add_subdirectory(imageioapphelpers)
endif()
