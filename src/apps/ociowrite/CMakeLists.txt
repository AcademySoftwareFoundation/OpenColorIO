# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    main.cpp
)

add_executable(ociowrite ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ociowrite
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(ociowrite PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(ociowrite
    PRIVATE 
        apputils
        OpenColorIO
)

install(TARGETS ociowrite
    RUNTIME DESTINATION bin
)

