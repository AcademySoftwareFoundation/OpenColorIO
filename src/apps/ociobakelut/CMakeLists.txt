# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    main.cpp
    ocioicc.cpp
)

add_executable(ociobakelut ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ociobakelut
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

if(MSVC)
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /wd4996")
endif()

# TODO: lcm2 2.2 does not compile on C++17 so revert to c++11

set_target_properties(ociobakelut
    PROPERTIES 
        COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}"
        C_STANDARD 11
        CXX_STANDARD 11
)

target_link_libraries(ociobakelut 
    PRIVATE 
        apputils
        lcms2::lcms2
        OpenColorIO
)

install(TARGETS ociobakelut
    RUNTIME DESTINATION bin
)
