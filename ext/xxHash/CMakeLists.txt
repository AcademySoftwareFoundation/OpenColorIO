# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# xxHash v0.8.1
# https://github.com/Cyan4973/xxHash
add_library(xxHash INTERFACE IMPORTED GLOBAL)
set_target_properties(xxHash PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src/include"
)
