set(SOURCES
	main.cpp
)

add_executable(ociolutimage ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ociolutimage
        PRIVATE
        OpenColorIO_SKIP_IMPORTS
    )
endif()

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
