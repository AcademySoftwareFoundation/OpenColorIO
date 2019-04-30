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

set_target_properties(ociolutimage PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(ociolutimage
	PRIVATE
		apputils
		pystring
		OpenColorIO
		OpenImageIO
)

install(TARGETS ociolutimage
	RUNTIME DESTINATION bin
)
