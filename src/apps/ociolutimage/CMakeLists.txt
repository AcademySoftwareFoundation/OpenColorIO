set(SOURCES
	main.cpp
)

add_executable(ociolutimage ${SOURCES})
target_link_libraries(ociolutimage
	PRIVATE
		apputils
		pystring
	PUBLIC
		OpenColorIO
		OpenImageIO
)

install(TARGETS ociolutimage
	RUNTIME DESTINATION bin
)
