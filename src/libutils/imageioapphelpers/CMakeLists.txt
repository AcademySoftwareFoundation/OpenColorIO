# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    imageio.cpp
)

add_library(imageioapphelpers STATIC ${SOURCES})
set_target_properties(imageioapphelpers PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(imageioapphelpers PROPERTIES OUTPUT_NAME OpenColorIOimageioapphelpers)

if (USE_MSVC AND OCIO_IMAGE_BACKEND STREQUAL "OpenImageIO")
	# Temporary until fixed in OpenImageIO: Mute some warnings from OpenImageIO farmhash.h
	# C4267 (level 3)	    'var' : conversion from 'size_t' to 'type', possible loss of data
	# C4244	(level 3 & 4)	'conversion' conversion from 'type1' to 'type2', possible loss of data
    target_compile_options(imageioapphelpers PRIVATE /wd4267 /wd4244)
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(imageioapphelpers
        PUBLIC
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(imageioapphelpers PROPERTIES
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}"
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