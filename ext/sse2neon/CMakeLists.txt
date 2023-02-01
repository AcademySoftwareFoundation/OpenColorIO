# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# sse2neon (modified)
# https://github.com/DLTcollab/sse2neon
add_library(sse2neon INTERFACE IMPORTED GLOBAL)
set_target_properties(sse2neon PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src/include"
)
