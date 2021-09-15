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
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(oiiohelpers PROPERTIES
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}"
)

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
