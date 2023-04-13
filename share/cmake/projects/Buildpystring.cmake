# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

project(pystring)

cmake_minimum_required(VERSION 3.10)

include(GNUInstallDirs)

set(HEADERS
    pystring.h
)

set(SOURCES
    pystring.cpp
)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

if(UNIX)
    set(pystring_CXX_FLAGS "${pystring_CXX_FLAGS};-fPIC")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES 
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};${pystring_CXX_FLAGS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
    PUBLIC_HEADER "${HEADERS}"
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pystring
)
