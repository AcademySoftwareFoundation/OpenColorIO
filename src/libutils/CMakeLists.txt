# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

add_subdirectory(apphelpers)

if(TARGET test_gpu_exec OR
   TARGET ociodisplay OR
   TARGET ociochecklut OR
   TARGET ocioconvert
)
    add_subdirectory(oglapphelpers)
endif()

if(TARGET ocioconvert OR
   TARGET ocioperf
)
    add_subdirectory(oiiohelpers)
endif()
