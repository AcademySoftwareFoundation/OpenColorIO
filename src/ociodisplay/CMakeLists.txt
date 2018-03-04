find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)
set(SOURCES
	main.cpp
)

add_executable(ociodisplay ${SOURCES})
target_include_directories(ociodisplay 
	SYSTEM
	PRIVATE
		${OIIO_INCLUDE_DIR}
		${OPENGL_INCLUDE_DIRS}
		${GLEW_INCLUDE_DIRS}
		${GLUT_INCLUDE_DIR}
)
target_link_libraries(ociodisplay
	PRIVATE 
		util
		openglbuilder
	PUBLIC
		OpenColorIO
		${OIIO_LIBRARIES}
		${OPENGL_LIBRARIES}
		${GLEW_LIBRARIES}
		${GLUT_LIBRARIES}
)
install(TARGETS ociodisplay
	RUNTIME DESTINATION bin
)
