# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(NOT OCIO_GL_ENABLED)
    message(WARNING "GL component missing. Skipping ociodisplay.")
    return()
endif()

set(SOURCES main.cpp)

add_executable(ociodisplay ${SOURCES})

set(CUSTOM_COMPILE_FLAGS ${PLATFORM_COMPILE_OPTIONS})
if(APPLE)
    # Mute the deprecated warning for some GLUT methods.
    set(CUSTOM_COMPILE_FLAGS "${CUSTOM_COMPILE_FLAGS};-DGL_SILENCE_DEPRECATION")
endif()

set_target_properties(ociodisplay PROPERTIES 
    COMPILE_OPTIONS "${CUSTOM_COMPILE_FLAGS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)

if (USE_MSVC)
	# Temporary until fixed in OpenImageIO: Mute some warnings from OpenImageIO farmhash.h
	# C4267 (level 3)	    'var' : conversion from 'size_t' to 'type', possible loss of data
	# C4244	(level 3 & 4)	'conversion' conversion from 'type1' to 'type2', possible loss of data
	target_compile_options(ociodisplay PRIVATE /wd4267 /wd4244)
endif()

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
        ${OPENGL_LIBRARIES}
        apputils
        imageioapphelpers
        oglapphelpers
        OpenColorIO
)

include(StripUtils)
ocio_strip_binary(ociodisplay)

install(TARGETS ociodisplay
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Set the debugger environment so that the executable can be launched
# directly within IDE (e.g. Visual Studio).
set_debugger_env(ociodisplay NEEDS_GL)
