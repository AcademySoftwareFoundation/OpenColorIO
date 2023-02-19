# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

project(lcms2)

cmake_minimum_required(VERSION 3.10)

include(GNUInstallDirs)

include_directories(include)

file(GLOB HEADERS "include/*.h")
file(GLOB SOURCES "src/*.c" "src/*.h")

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

if(UNIX)
    set(lcms2_C_FLAGS "${lcms2_C_FLAGS};-fPIC")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_NAME "${PROJECT_NAME}"
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};${lcms2_C_FLAGS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
    PUBLIC_HEADER "${HEADERS}"
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lcms2
)
