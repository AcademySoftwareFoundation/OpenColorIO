# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(OPENFX_SRC "${CMAKE_SOURCE_DIR}/vendor/openfx/openfx")
set(OPENFX_INCLUDE "${OPENFX_SRC}/include")
set(RES_DIR "${CMAKE_SOURCE_DIR}/share/openfx/resources")
set(PLUGIN_NAME "OCIOPlugins")

if(NOT EXISTS ${OPENFX_INCLUDE})
    message(FATAL_ERROR "\nOpenFX not found! Issue 'git submodule update' and try again.\n")
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${OPENFX_INCLUDE}
    ${CMAKE_SOURCE_DIR}/export
    ${CMAKE_SOURCE_DIR}/build/include/OpenColorIO
    ${CMAKE_BINARY_DIR}/include
)

file(GLOB LIB
    ${CMAKE_BINARY_DIR}/src/OpenColorIO/libOpenColorIO2.0.0.dylib
    ${CMAKE_BINARY_DIR}/src/OpenColorIO/libOpenColorIO2.0.dylib
    ${CMAKE_BINARY_DIR}/src/OpenColorIO/libOpenColorIO.dylib
)

file(GLOB SRC
    "ColorSpaceTransformPlugin/ColorSpaceTransformPlugin.cpp"
    "FileTransformPlugin/FileTransformPlugin.cpp"
    "DisplayViewTransformPlugin/DisplayViewTransform.cpp"
    "PluginMain.cpp"
)

file(GLOB RESOURCES
    ${RES_DIR}/ocio.svg
    ${CMAKE_SOURCE_DIR}/LICENSE
    ${CMAKE_SOURCE_DIR}/README.md
)

file(GLOB PLIST
    ${CMAKE_SOURCE_DIR}/share/openfx/Info.plist
)

add_library(${PLUGIN_NAME} SHARED ${SRC})

set_target_properties(
    ${PLUGIN_NAME}
    PROPERTIES PREFIX ""
    SUFFIX ".ofx"
)

target_compile_definitions(
    ${PLUGIN_NAME}
    PRIVATE
    OFX_EXTENSIONS_OCIO
)

# Linking against OCIO dylibs
target_link_libraries(
    ${PLUGIN_NAME}
    ${LIB}
)

# Finding the architecture
set(OFX_ARCH UNKNOWN)
# Win
if(MINGW)
    set(OFX_ARCH "Win32")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "Win64")
    endif()
# Visual Studio
elseif(MSVC)
    set(OFX_ARCH "Win32")
    if((${CMAKE_SYSTEM_PROCESSOR} STREQUAL "IA64") OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64"))
        set(OFX_ARCH "Win64")
    endif()
# FreeBSD
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(OFX_ARCH "FreeBSD-x86")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "FreeBSD-x86-64")
    endif()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(OFX_ARCH FreeBSD-arm-64)
        else()
            set(OFX_ARCH FreeBSD-arm)
        endif()
    endif()
# MacOS
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(OFX_ARCH "MacOS")
# Linux
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(OFX_ARCH "Linux-x86")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "Linux-x86-64")
    endif()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(OFX_ARCH Linux-arm-64)
        else()
            set(OFX_ARCH Linux-arm)
        endif()
    endif()
endif()

set(OFX_ARCH_NAME OFX_ARCH CACHE STRING "OpenFX target OS and architecture")
if(OFX_ARCH_NAME EQUAL "UNKNOWN")
    message(SEND_ERROR "OFX_ARCH_NAME was not detected. Install targets will be unavailable. See http://openeffects.org/documentation/reference/ch02s02.html to pick an arch name.")
else()
    set(PLUGIN_CONTENTS_DIR ${CMAKE_BINARY_DIR}/vendor/openfx/${PLUGIN_NAME}.ofx.bundle/Contents)
    set(PLUGIN_LIB_DIR ${PLUGIN_CONTENTS_DIR}/${OFX_ARCH_NAME})
    set(PLUGIN_RES_DIR ${PLUGIN_CONTENTS_DIR}/Resources)
    install(TARGETS ${PLUGIN_NAME}
        DESTINATION ${PLUGIN_LIB_DIR}
    )
    install(FILES ${RESOURCES}
        DESTINATION ${PLUGIN_RES_DIR}
    )
    install(FILES ${PLIST}
        DESTINATION ${PLUGIN_CONTENTS_DIR}
    )
endif()
