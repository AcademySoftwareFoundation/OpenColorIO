# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Install DirectX-Headers (header-only, Windows only)
# https://github.com/microsoft/DirectX-Headers
#
###############################################################################

include(FetchContent)

set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/ext/build/DirectX-Headers")
set(DIRECTX_HEADERS_BUILD_TEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(DirectX-Headers
    GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
    GIT_TAG        v1.619.1
)

FetchContent_MakeAvailable(DirectX-Headers)

# Signal success to ocio_install_dependency so ocio_handle_dependency does not
# abort at the next required-check. FetchContent_MakeAvailable has just created
# the Microsoft::DirectX-Headers target via the upstream CMakeLists.
if(TARGET Microsoft::DirectX-Headers)
    set(DirectX-Headers_FOUND TRUE)
    set(DirectX-Headers_VERSION "1.619.1")
endif()
