# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    main.cpp
)

add_executable(ociomergeconfigs ${SOURCES})

set_target_properties(ociomergeconfigs PROPERTIES 
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)

target_include_directories(ociomergeconfigs
    PUBLIC
        "$<BUILD_INTERFACE:${BUILD_INCLUDES}>"
)

target_link_libraries(ociomergeconfigs
    PRIVATE
        apputils
        pystring::pystring
        OpenColorIO
)

include(StripUtils)
ocio_strip_binary(ociomergeconfigs)

install(TARGETS ociomergeconfigs
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Set the debugger environment so that the executable can be launched
# directly within IDE (e.g. Visual Studio).
set_debugger_env(ociomergeconfigs)
