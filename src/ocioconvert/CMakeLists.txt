find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)
set(SOURCES
	main.cpp
)

add_executable(ocioconvert ${SOURCES})
target_include_directories(ocioconvert 
	SYSTEM
	PRIVATE
		${OIIO_INCLUDE_DIR} 
		${GLEW_INCLUDE_DIRS}
		${GLUT_INCLUDE_DIR}
)
target_link_libraries(ocioconvert
	PRIVATE 
		util
	PUBLIC
		OpenColorIO
		${OIIO_LIBRARIES}
		${GLEW_LIBRARIES}
		${GLUT_LIBRARIES}
)
install(TARGETS ocioconvert
	RUNTIME DESTINATION bin
)

