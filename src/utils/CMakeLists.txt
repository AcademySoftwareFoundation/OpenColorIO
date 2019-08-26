# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(TARGET test_gpu_exec OR 
   TARGET ociodisplay OR
   TARGET ocioconvert
)
    add_subdirectory(oglbuilder)
endif()

if(TARGET ocioconvert OR 
   TARGET ocioperf
)
    add_subdirectory(oiiohelpers)
endif()
