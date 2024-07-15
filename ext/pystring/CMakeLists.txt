# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# pystring v1.1.4
# https://github.com/imageworks/pystring/blob/master/pystring.h
add_library(pystring INTERFACE IMPORTED GLOBAL)
set_target_properties(pystring PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src/include"
)
