# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    CategoryHelpers.cpp
    ColorSpaceHelpers.cpp
    DisplayViewHelpers.cpp
    MixingHelpers.cpp
)

set(INCLUDES
    CategoryNames.h
    ColorSpaceHelpers.h
    DisplayViewHelpers.h
    MixingHelpers.h
)

# Some include files are private to the library.
set(PRIVATE_INCLUDES
    CategoryHelpers.h
)

add_library(apphelpers STATIC ${SOURCES})
set_target_properties(apphelpers PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(apphelpers PROPERTIES OUTPUT_NAME OpenColorIOapphelpers)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(apphelpers
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(apphelpers PROPERTIES
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_include_directories(apphelpers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(apphelpers
    PRIVATE
        OpenColorIO
        utils::strings
)

install(TARGETS apphelpers
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES  ${INCLUDES}
    DESTINATION include/OpenColorIO/apphelpers
)
