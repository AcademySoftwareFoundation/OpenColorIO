set(SOURCES
	main.cpp
)

add_executable(ociolutimage ${SOURCES})
target_include_directories(ociolutimage
	SYSTEM
	PRIVATE
		${OIIO_INCLUDE_DIR}
)
target_link_libraries(ociolutimage
	PRIVATE
		util
		pystring
	PUBLIC
		OpenColorIO
		${OIIO_LIBRARIES}
)

install(TARGETS ociolutimage
	RUNTIME DESTINATION bin
)
