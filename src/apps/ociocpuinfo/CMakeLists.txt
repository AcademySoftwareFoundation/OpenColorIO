# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    main.cpp
    ${PROJECT_SOURCE_DIR}/src/OpenColorIO/CPUInfo.cpp
)

add_executable(ociocpuinfo ${SOURCES})

set_target_properties(ociocpuinfo PROPERTIES
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)

target_include_directories(ociocpuinfo
    PRIVATE
        "$<TARGET_PROPERTY:OpenColorIO,INCLUDE_DIRECTORIES>"
        "${PROJECT_BINARY_DIR}/generated_include"
)

include(StripUtils)
ocio_strip_binary(ociocpuinfo)

install(TARGETS ociocpuinfo
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
