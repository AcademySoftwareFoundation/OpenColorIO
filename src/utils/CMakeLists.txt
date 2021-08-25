# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# Note: Header only to avoid explicit dependencies.

add_library(utils::strings INTERFACE IMPORTED GLOBAL)

set_target_properties(utils::strings PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

# Configure half.h include location (Imath or IlmBase)
configure_file("Half.h.in" "Half.h" @ONLY)

set_property(TARGET ${OCIO_HALF_LIB} APPEND PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/.."
)
