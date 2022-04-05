# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    main.cpp
)

add_executable(ociowrite ${SOURCES})

set_target_properties(ociowrite PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(ociowrite
    PRIVATE 
        apputils
        OpenColorIO
)

install(TARGETS ociowrite
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
