# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    main.cpp
)

add_executable(ocioarchive ${SOURCES})

set_target_properties(ocioarchive PROPERTIES 
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)

target_include_directories(ocioarchive
    PUBLIC
        "$<BUILD_INTERFACE:${BUILD_INCLUDES}>"
)

target_link_libraries(ocioarchive
    PRIVATE
        apputils
        OpenColorIO
        MINIZIP::minizip-ng
)

include(StripUtils)
ocio_strip_binary(ocioarchive)

install(TARGETS ocioarchive
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Set the debugger environment so that the executable can be launched
# directly within IDE (e.g. Visual Studio).
set_debugger_env(ocioarchive)
