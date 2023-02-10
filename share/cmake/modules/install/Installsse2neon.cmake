# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Install sse2neon (header-only version)
# https://github.com/DLTcollab/sse2neon
#
#
# Global targets defined by this module:
#   sse2neon
###############################################################################

# Download sse2neon using FetchContent and make it available at configure time.

include(FetchContent)

set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/ext/build/sse2neon")
FetchContent_Declare(sse2neon
  GIT_REPOSITORY https://github.com/DLTcollab/sse2neon.git
  GIT_TAG        v1.6.0
)

# FetchContent_MakeAvailable is not available until CMake 3.14+.
# Using FetchContent_GetProperties and FetchContent_Populate instead.
FetchContent_GetProperties(sse2neon)

if(NOT sse2neon_POPULATED)
  FetchContent_Populate(sse2neon)

  set(_EXT_DIST_INCLUDE "${CMAKE_BINARY_DIR}/ext/dist/${CMAKE_INSTALL_INCLUDEDIR}")
  file(COPY "${sse2neon_SOURCE_DIR}/sse2neon.h" DESTINATION "${_EXT_DIST_INCLUDE}/sse2neon")

  add_library(sse2neon INTERFACE IMPORTED GLOBAL)
  set_target_properties(sse2neon PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${_EXT_DIST_INCLUDE}/sse2neon"
  )
endif()