# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    main.cpp
)

add_executable(ocioarchive ${SOURCES})

set_target_properties(ocioarchive PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_include_directories(ocioarchive
    PUBLIC
        "$<BUILD_INTERFACE:${BUILD_INCLUDES}>"
)

target_link_libraries(ocioarchive
    PRIVATE
        apputils
        OpenColorIO
        minizip-ng::minizip-ng
)

install(TARGETS ocioarchive
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
