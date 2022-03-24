# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    main.cpp
)

add_executable(ociocheck ${SOURCES})

if(MSVC)
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /wd4996")
endif()

set_target_properties(ociocheck PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(ociocheck 
    PRIVATE 
        apputils
        OpenColorIO
)

install(TARGETS ociocheck
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
