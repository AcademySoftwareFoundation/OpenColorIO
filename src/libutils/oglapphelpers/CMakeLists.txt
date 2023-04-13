# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(NOT OCIO_GL_ENABLED)
    message(WARNING "GL component missing. Skipping oglapphelpers.")
    return()
endif()

set(SOURCES
    glsl.cpp
    oglapp.cpp
)
set(INCLUDES
    glsl.h
    oglapp.h
)

if(APPLE)

    list(APPEND SOURCES
        msl.mm
        mtltexture.mm
        metalapp.mm
    )

    list(APPEND INCLUDES
        msl.h
        mtltexture.h
        metalapp.h
    )

endif()

add_library(oglapphelpers STATIC ${SOURCES})
set_target_properties(oglapphelpers PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(oglapphelpers PROPERTIES OUTPUT_NAME OpenColorIOoglapphelpers)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(oglapphelpers
        PUBLIC
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set(CUSTOM_COMPILE_FLAGS ${PLATFORM_COMPILE_OPTIONS})
if(APPLE)
    # Mute the deprecated warning for some GLUT methods.
    set(CUSTOM_COMPILE_FLAGS "${CUSTOM_COMPILE_FLAGS};-DGL_SILENCE_DEPRECATION")
endif()

set_target_properties(oglapphelpers PROPERTIES 
    COMPILE_OPTIONS "${CUSTOM_COMPILE_FLAGS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)

target_include_directories(oglapphelpers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIRS}
        ${GLUT_INCLUDE_DIR}
)


if(${OCIO_USE_GLVND})
    if(${OCIO_EGL_HEADLESS})
        target_include_directories(oglapphelpers
            PRIVATE
                ${OPENGL_EGL_INCLUDE_DIRS}
        )
        target_link_libraries(oglapphelpers
            PRIVATE
                OpenColorIO
                OpenGL::OpenGL
                OpenGL::GLU
                ${GLEW_LIBRARIES}
                ${GLUT_LIBRARIES}
                OpenGL::EGL
            )
    else()
        target_link_libraries(oglapphelpers
            PRIVATE
                OpenColorIO
                OpenGL::OpenGL
                OpenGL::GLU
                ${GLEW_LIBRARIES}
                ${GLUT_LIBRARIES}
           )
   endif()
else()
    # if OCIO_USE_GLVND is OFF, OCIO_EGL_HEADLESS is also OFF
    target_link_libraries(oglapphelpers
        PRIVATE
            OpenColorIO
            ${OPENGL_LIBRARIES}
            ${GLEW_LIBRARIES}
            ${GLUT_LIBRARIES}
        )
endif()

if(APPLE)
    target_link_libraries(oglapphelpers
        PRIVATE
            "-framework Carbon"
            "-framework IOKit"
            "-framework Metal"
            "-framework CoreVideo"
    )
endif()

if(${OCIO_EGL_HEADLESS})
    target_include_directories(oglapphelpers
        PRIVATE
            ${OPENGL_EGL_INCLUDE_DIRS}
    )
endif()

install(TARGETS oglapphelpers 
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${INCLUDES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenColorIO/oglapphelpers
)
