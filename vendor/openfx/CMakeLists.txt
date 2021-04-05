# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    OCIOColorSpace.cpp
    OCIOMain.cpp
    OCIOProcessor.cpp
    OCIOUtils.cpp
)
set(OFXS_SOURCES
    ${openfx_SUPPORT_DIR}/Library/ofxsCore.cpp
    ${openfx_SUPPORT_DIR}/Library/ofxsImageEffect.cpp
    ${openfx_SUPPORT_DIR}/Library/ofxsInteract.cpp
    ${openfx_SUPPORT_DIR}/Library/ofxsLog.cpp
    ${openfx_SUPPORT_DIR}/Library/ofxsMultiThread.cpp
    ${openfx_SUPPORT_DIR}/Library/ofxsParams.cpp
    ${openfx_SUPPORT_DIR}/Library/ofxsProperty.cpp
    ${openfx_SUPPORT_DIR}/Library/ofxsPropertyValidation.cpp
)
set(PLIST
    ${CMAKE_SOURCE_DIR}/share/openfx/Info.plist
)
set(RESOURCES
    ${CMAKE_SOURCE_DIR}/share/openfx/resources/OpenColorIO.OCIOColorSpace.png
    ${CMAKE_SOURCE_DIR}/share/openfx/resources/OpenColorIO.OCIOColorSpace.svg
)

add_library(ofxplugin MODULE ${SOURCES} ${OFXS_SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ofxplugin
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

# Needed to define OpenFX Support Library visibility macros on Linux
if(UNIX AND NOT APPLE)
    target_compile_definitions(ofxplugin 
        PRIVATE
            linux
    )
endif()

# These source files may be a future byproduct of OCIO_INSTALL_EXT_PACKAGES. 
# Skip existence validation unless an explicit install location is provided.
if(NOT DEFINED openfx_ROOT)
    set_source_files_properties(${OFXS_SOURCES} PROPERTIES 
        GENERATED TRUE)
endif()

# Disable known compiler warnings from OpenFX Support Library
if(MSVC)
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /wd4996 /wd4101")
else()
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Wno-deprecated \
                                                          -Wno-deprecated-declarations \
                                                          -Wno-switch-enum \
                                                          -Wno-dynamic-exception-spec \
                                                          -Wno-catch-value \
                                                          -Wno-unknown-warning-option")
endif()

# OpenFX Support Library source doesn't currently support C++17
set(OFX_CXX_STANDARD ${CMAKE_CXX_STANDARD})
if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 17)
    set(OFX_CXX_STANDARD 11)
endif()

set_target_properties(ofxplugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "OpenColorIO"
    SUFFIX ".ofx"
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}"
    CXX_STANDARD ${OFX_CXX_STANDARD}
)

target_include_directories(ofxplugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${openfx_INCLUDE_DIR}
        ${openfx_SUPPORT_DIR}/include
        ${openfx_SUPPORT_DIR}/Plugins/include
        ${CMAKE_SOURCE_DIR}/include/
        ${CMAKE_BINARY_DIR}/include/
)

target_link_libraries(ofxplugin
    PRIVATE
        openfx::module
        OpenColorIO
)

# Detect OFX architecture
if(WIN32)
    set(OFX_ARCH "Win32")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(OFX_ARCH "Win64")
    endif()
elseif(APPLE)
    set(OFX_ARCH "MacOS")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(OFX_ARCH "MacOS-x86-64")
    endif()
elseif(UNIX)
    set(OFX_ARCH "Linux-x86")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(OFX_ARCH "Linux-x86-64")
    endif()
else()
    message(FATAL_ERROR 
            "The OpenColorIO OFX plugins are unsupported for the current system architecture")
endif()

# Install to OFX package structure
set(OFX_INSTALL_PREFIX "lib/OpenColorIO.ofx.bundle/Contents")

install(DIRECTORY 
    DESTINATION ${OFX_INSTALL_PREFIX}
)
install(FILES ${PLIST}
    DESTINATION ${OFX_INSTALL_PREFIX}
)
install(FILES ${RESOURCES}
    DESTINATION ${OFX_INSTALL_PREFIX}/Resources
)
install(TARGETS ofxplugin LIBRARY 
    DESTINATION ${OFX_INSTALL_PREFIX}/${OFX_ARCH}
)
