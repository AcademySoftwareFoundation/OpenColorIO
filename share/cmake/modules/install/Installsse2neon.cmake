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

  # sse2neon_INCLUDE_DIR is used internally for CheckSupportSSEUsingSSE2NEON.cmake and to create sse2neon
  # target for OCIO.
  set(sse2neon_INCLUDE_DIR "${sse2neon_SOURCE_DIR}")

  # Any changes to the following lines must be replicated in ./CMakeLists.txt as well.
  # Create a target for sse2neon (non-imported)
  add_library(sse2neon INTERFACE)
  # Add the include directories to the target.
  target_include_directories(sse2neon INTERFACE "${sse2neon_INCLUDE_DIR}")
  # Ignore the warnings coming from sse2neon.h as they are false positives.
  target_compile_options(sse2neon INTERFACE -Wno-unused-parameter)
endif()