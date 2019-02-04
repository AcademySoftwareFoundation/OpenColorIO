find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

set(SOURCES
	glsl.cpp
)

add_library(openglbuilder STATIC ${SOURCES})
set_target_properties(openglbuilder PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(openglbuilder
        PRIVATE
        OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(openglbuilder PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_include_directories(openglbuilder
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE
		${OPENGL_INCLUDE_DIR}
		${GLEW_INCLUDE_DIRS}
)
target_link_libraries(openglbuilder
	PRIVATE
		${OPENGL_LIBRARIES}
		${GLEW_LIBRARIES}
	PUBLIC
		public_api
)
