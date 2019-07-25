find_package(OpenGL REQUIRED)
if(NOT APPLE)
    find_package(GLEW REQUIRED)
endif()

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
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_include_directories(oiiohelpers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(oiiohelpers
    PUBLIC
        public_api
        OpenColorIO
        OpenImageIO

    PRIVATE
        ilmbase
        pystring
)

install(TARGETS oiiohelpers 
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/oiiohelpers.h
    DESTINATION include/OpenColorIO/oiiohelpers
)
