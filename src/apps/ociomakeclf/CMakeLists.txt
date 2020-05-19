# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    main.cpp
)

add_executable(ociomakeclf ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ociomakeclf
        PRIVATE
        OpenColorIO_SKIP_IMPORTS
    )
endif()

if(MSVC)
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /wd4996")
endif()

set_target_properties(ociomakeclf PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(ociomakeclf 
    PRIVATE 
        apputils
        OpenColorIO
        utils::strings
)

install(TARGETS ociomakeclf
    RUNTIME DESTINATION bin
)
