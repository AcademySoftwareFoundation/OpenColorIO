# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# half from imath (stripped down to header-only implementation)
# https://github.com/AcademySoftwareFoundation/Imath
add_library(half INTERFACE IMPORTED GLOBAL)
set_target_properties(half PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src/include"
)
