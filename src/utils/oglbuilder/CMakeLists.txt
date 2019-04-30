find_package(OpenGL REQUIRED)
if(NOT APPLE)
    find_package(GLEW REQUIRED)
endif()

set(SOURCES
    glsl.cpp
)

add_library(oglbuilder STATIC ${SOURCES})
set_target_properties(oglbuilder PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(oglbuilder PROPERTIES OUTPUT_NAME OpenColorIOoglbuilder)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(oglbuilder
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(oglbuilder PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_include_directories(oglbuilder
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIRS}
)

target_link_libraries(oglbuilder
    PUBLIC
        public_api
    PRIVATE
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
)

install(TARGETS oglbuilder 
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/glsl.h
    DESTINATION include/OpenColorIO/oglbuilder
)
