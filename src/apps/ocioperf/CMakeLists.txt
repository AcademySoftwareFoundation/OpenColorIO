# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    main.cpp
)

add_executable(ocioperf ${SOURCES})

set_target_properties(ocioperf PROPERTIES
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)

target_link_libraries(ocioperf
    PRIVATE
        apputils
        OpenColorIO
        utils::strings
)

include(StripUtils)
ocio_strip_binary(ocioperf)

install(TARGETS ocioperf
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Set the debugger environment so that the executable can be launched
# directly within IDE (e.g. Visual Studio).
set_debugger_env(ocioperf)
