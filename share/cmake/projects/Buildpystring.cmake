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

# pystring has no pkg-config file upstream, unlike its OCIO_INSTALL_EXT_PACKAGES
# siblings (expat, yaml-cpp, Imath, minizip-ng, zlib); write a minimal one so
# OpenColorIO.pc's Requires.private can reference it like the others.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/pystring.pc"
"prefix=${CMAKE_INSTALL_PREFIX}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}
includedir=\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}/pystring

Name: pystring
Description: A C++ port of some of Python's string functions
Version: ${PYSTRING_PC_VERSION}
Libs: -L\${libdir} -lpystring
Cflags: -I\${includedir}
")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pystring.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
