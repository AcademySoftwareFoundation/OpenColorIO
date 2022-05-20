# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    oiiohelpers.cpp
)

add_library(oiiohelpers STATIC ${SOURCES})
set_target_properties(oiiohelpers PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(oiiohelpers PROPERTIES OUTPUT_NAME OpenColorIOoiiohelpers)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(oiiohelpers
        PUBLIC
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(oiiohelpers PROPERTIES
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}"
)

if (USE_MSVC)
	# Temporary until fixed in OpenImageIO: Mute some warnings from OpenImageIO farmhash.h
	# C4267 (level 3)	    'var' : conversion from 'size_t' to 'type', possible loss of data
	# C4244	(level 3 & 4)	'conversion' conversion from 'type1' to 'type2', possible loss of data
	target_compile_options(oiiohelpers PRIVATE /wd4267 /wd4244)
endif()

target_include_directories(oiiohelpers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(oiiohelpers
    PUBLIC
        OpenImageIO::OpenImageIO

    PRIVATE
        OpenColorIO
        utils::strings
)

install(TARGETS oiiohelpers
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/oiiohelpers.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenColorIO/oiiohelpers
)
