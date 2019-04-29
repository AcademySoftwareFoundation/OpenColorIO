find_package(OpenGL REQUIRED)
if(NOT APPLE)
	find_package(GLEW REQUIRED)
endif()
find_package(GLUT REQUIRED)

set(SOURCES
	main.cpp
)

add_executable(ocioconvert ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ocioconvert
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(ocioconvert PROPERTIES 
	COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_include_directories(ocioconvert 
	SYSTEM
	PRIVATE
		${OPENGL_INCLUDE_DIR}
		${GLEW_INCLUDE_DIRS}
		${GLUT_INCLUDE_DIR}
)
target_link_libraries(ocioconvert
	PRIVATE 
		apputils
		oglbuilder
		OpenColorIO
		OpenImageIO
		${OPENGL_LIBRARIES}
		${GLEW_LIBRARIES}
		${GLUT_LIBRARIES}
)
install(TARGETS ocioconvert
	RUNTIME DESTINATION bin
)

