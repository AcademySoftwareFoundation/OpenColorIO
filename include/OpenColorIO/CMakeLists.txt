# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

configure_file("OpenColorABI.h.in" "${CMAKE_CURRENT_BINARY_DIR}/OpenColorABI.h" @ONLY)
add_library(OpenColorIOHeaders INTERFACE)

set(BUILD_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO
)

target_include_directories(OpenColorIOHeaders
    INTERFACE
        "$<BUILD_INTERFACE:${BUILD_INCLUDES}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

add_dependencies(OpenColorIOHeaders "${CMAKE_CURRENT_BINARY_DIR}/OpenColorABI.h")

# public interface

file(GLOB_RECURSE core_export_headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
list(APPEND core_export_headers ${CMAKE_CURRENT_BINARY_DIR}/OpenColorABI.h)

install(FILES ${core_export_headers}
    DESTINATION include/OpenColorIO)

install(TARGETS OpenColorIOHeaders
    EXPORT ${PROJECT_NAME}_EXPORTED_TARGETS
)
