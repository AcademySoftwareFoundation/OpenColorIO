# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# Note: Header only to avoid explicit dependencies.

add_library(utils::strings INTERFACE IMPORTED GLOBAL)

set_target_properties(utils::strings PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

# from_chars shim (via strtod_l)

add_library(utils::from_chars INTERFACE IMPORTED GLOBAL)

set_target_properties(utils::from_chars PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
