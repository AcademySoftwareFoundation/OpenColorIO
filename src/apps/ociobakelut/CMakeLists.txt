# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    main.cpp
    ocioicc.cpp
)

add_executable(ociobakelut ${SOURCES})

if(MSVC)
    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};/wd4996")
endif()

# NOTE: Depending of the compiler version lcm2 2.2 does not compile with C++17 so revert to c++11

set(APP_CXX_STANDARD ${CMAKE_CXX_STANDARD})
if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 17)
    set(APP_CXX_STANDARD 11)
endif()

set_target_properties(ociobakelut
    PROPERTIES 
        COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
        LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
        CXX_STANDARD ${APP_CXX_STANDARD}
)

target_link_libraries(ociobakelut 
    PRIVATE 
        apputils
        lcms2::lcms2
        OpenColorIO
)

include(StripUtils)
ocio_strip_binary(ociobakelut)

install(TARGETS ociobakelut
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Set the debugger environment so that the executable can be launched
# directly within IDE (e.g. Visual Studio).
set_debugger_env(ociobakelut)
