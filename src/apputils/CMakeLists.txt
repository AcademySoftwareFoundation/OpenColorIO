# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    argparse.cpp
    strutil.cpp
    logGuard.cpp
)

add_library(apputils STATIC ${SOURCES})

target_include_directories(apputils 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(apputils
    PRIVATE
        OpenColorIO
        pystring::pystring
)

set_target_properties(apputils PROPERTIES 
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)
