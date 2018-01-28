find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

set(SOURCES
	glsl.cpp
)

add_library(openglbuilder STATIC ${SOURCES})
set_target_properties(openglbuilder PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(openglbuilder
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE
		${GLEW_INCLUDE_DIRS}
)
target_link_libraries(openglbuilder
	PRIVATE
		${OPENGL_LIBRARIES}
		${GLEW_LIBRARIES}
	PUBLIC
		public_api
)
