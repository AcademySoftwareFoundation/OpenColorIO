if (OIIO_FOUND)

	# If dependencies are missing turn off GPU
	if(NOT (OPENGL_FOUND AND GLUT_FOUND AND GLEW_FOUND))
		set(OCIO_BUILD_CONVERT_WITH_GPU OFF)
	endif()

	include_directories(
		${CMAKE_SOURCE_DIR}/export/
		${CMAKE_BINARY_DIR}/export/
		${CMAKE_SOURCE_DIR}/src/apps/share/
		${OIIO_INCLUDES}
		${ILMBASE_INCLUDES}
	)

	if(OCIO_BUILD_CONVERT_WITH_GPU)
		include_directories(
			${CMAKE_SOURCE_DIR}/src/lang-glsl/
			${OPENGL_INCLUDE_DIR}
			${GLUT_INCLUDE_DIR}
			${GLEW_INCLUDES}
		)
	endif()
    
    file(GLOB_RECURSE share_src_files "${CMAKE_SOURCE_DIR}/src/apps/share/*.cpp")
    
    add_executable(ocioconvert ${share_src_files} main.cpp)
    
    set_target_properties(ocioconvert PROPERTIES COMPILE_FLAGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

	target_link_libraries(ocioconvert ${OIIO_LIBRARIES} ${CMAKE_DL_LIBS})

	if(OCIO_BUILD_CONVERT_WITH_GPU)
		add_definitions("-DBUILD_WITH_GPU")
		target_link_libraries(ocioconvert OCIOOpenGL ${GLEW_LIBRARIES} ${GLUT_LIBRARY} ${OPENGL_LIBRARY})
	endif()
    
    target_link_OCIO(ocioconvert)
    
    install(TARGETS ocioconvert EXPORT OpenColorIO DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/bin)
endif()
