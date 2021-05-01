# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(NOT OCIO_GL_ENABLED)
    message(WARNING "GL component missing. Skipping ociodisplay.")
    return()
endif()

set(SOURCES
    main.cpp
)

add_executable(ociodisplay ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ociodisplay
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set(CUSTOM_COMPILE_FLAGS ${PLATFORM_COMPILE_FLAGS})
if(APPLE)
    # Mute the deprecated warning for some GLUT methods.
    set(CUSTOM_COMPILE_FLAGS "${CUSTOM_COMPILE_FLAGS} -DGL_SILENCE_DEPRECATION")
endif()

set_target_properties(ociodisplay PROPERTIES 
	COMPILE_FLAGS "${CUSTOM_COMPILE_FLAGS}")

target_include_directories(ociodisplay 
	SYSTEM
	PRIVATE
		${GLEW_INCLUDE_DIRS}
		${GLUT_INCLUDE_DIR}
		${OPENGL_INCLUDE_DIR}
)
target_link_libraries(ociodisplay
	PRIVATE 
		${GLEW_LIBRARIES}
		${GLUT_LIBRARIES}
		oglapphelpers
		OpenColorIO
		${OPENGL_LIBRARIES}
		OpenImageIO::OpenImageIO
)
install(TARGETS ociodisplay
    RUNTIME DESTINATION bin
)
