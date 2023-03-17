# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    imageio.cpp
)

add_library(imageioapphelpers STATIC ${SOURCES})
set_target_properties(imageioapphelpers PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(imageioapphelpers PROPERTIES OUTPUT_NAME OpenColorIOimageioapphelpers)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(imageioapphelpers
        PUBLIC
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(imageioapphelpers PROPERTIES
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)

target_compile_definitions(imageioapphelpers
    PRIVATE
        $<$<STREQUAL:"${OCIO_IMAGE_BACKEND}","OpenEXR">:USE_OPENEXR>
        $<$<STREQUAL:"${OCIO_IMAGE_BACKEND}","OpenImageIO">:USE_OPENIMAGEIO>
)

target_include_directories(imageioapphelpers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(imageioapphelpers
    PUBLIC
        OpenColorIO::ImageIOBackend
    PRIVATE
        OpenColorIO
        utils::strings
)

install(TARGETS imageioapphelpers
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/imageio.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenColorIO/imageioapphelpers
)