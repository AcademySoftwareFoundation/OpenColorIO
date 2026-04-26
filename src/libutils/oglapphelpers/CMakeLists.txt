# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(NOT OCIO_GL_ENABLED AND NOT (WIN32 AND OCIO_DIRECTX_ENABLED) AND NOT OCIO_VULKAN_ENABLED)
    message(WARNING "GL component missing, DirectX disabled, and Vulkan disabled. Skipping oglapphelpers.")
    return()
endif()

set(SOURCES
    graphicalapp.cpp
)
set(INCLUDES
    graphicalapp.h
)

if(OCIO_GL_ENABLED)
    list(APPEND SOURCES
        glsl.cpp
        oglapp.cpp
    )
    list(APPEND INCLUDES
        glsl.h
        oglapp.h
    )
endif()

if(WIN32 AND OCIO_DIRECTX_ENABLED)
    list(APPEND SOURCES
        dxapp.cpp
        hlsl.cpp
    )
    list(APPEND INCLUDES
        dxapp.h
        dxutils.h
        hlsl.h
    )
endif()

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

if(OCIO_VULKAN_ENABLED)

    find_package(Vulkan REQUIRED)
    find_package(glslang REQUIRED)

    list(APPEND SOURCES
        vulkanapp.cpp
    )

    list(APPEND INCLUDES
        vulkanapp.h
    )

endif()

add_library(oglapphelpers STATIC ${INCLUDES} ${SOURCES})
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
)

if(OCIO_GL_ENABLED)
    target_include_directories(oglapphelpers
        PRIVATE
            ${OPENGL_INCLUDE_DIR}
            ${GLEW_INCLUDE_DIRS}
            ${GLUT_INCLUDE_DIR}
    )
endif()

target_link_libraries(oglapphelpers
    PRIVATE
        OpenColorIO
)

if(OCIO_GL_ENABLED)
    if(${OCIO_USE_GLVND})
        if(${OCIO_EGL_HEADLESS})
            target_include_directories(oglapphelpers
                PRIVATE
                    ${OPENGL_EGL_INCLUDE_DIRS}
            )
            target_link_libraries(oglapphelpers
                PRIVATE
                    OpenGL::OpenGL
                    OpenGL::GLU
                    ${GLEW_LIBRARIES}
                    ${GLUT_LIBRARIES}
                    OpenGL::EGL
                )
        else()
            target_link_libraries(oglapphelpers
                PRIVATE
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
                ${OPENGL_LIBRARIES}
                ${GLEW_LIBRARIES}
                ${GLUT_LIBRARIES}
            )
    endif()
endif()

if(APPLE)
    target_link_libraries(oglapphelpers
        PRIVATE
            "-framework ColorSync"
            "-framework CoreFoundation"
            "-framework CoreGraphics"
            "-framework IOKit"
            "-framework Metal"
            "-framework CoreVideo"
    )
endif()

if(OCIO_GL_ENABLED)
    target_compile_definitions(oglapphelpers
        PUBLIC
            OCIO_GL_ENABLED
    )
endif()

if(WIN32 AND OCIO_DIRECTX_ENABLED)
    include(ocio_handle_dependency)
    ocio_handle_dependency(DirectX-Headers REQUIRED ALLOW_INSTALL
                           MIN_VERSION 1.606.0
                           RECOMMENDED_VERSION 1.619.1
                           RECOMMENDED_VERSION_REASON "Latest version tested with OCIO")
    target_compile_definitions(oglapphelpers
        PUBLIC
            OCIO_DIRECTX_ENABLED
    )
    target_link_libraries(oglapphelpers
        PUBLIC
            Microsoft::DirectX-Headers
        PRIVATE
            d3d12
            dxgi
            dxcompiler
            dxguid
    )
endif()

if(OCIO_VULKAN_ENABLED)
    target_include_directories(oglapphelpers
        PUBLIC
            ${Vulkan_INCLUDE_DIRS}
    )
    target_link_libraries(oglapphelpers
        PUBLIC
            Vulkan::Vulkan
            glslang::glslang
            glslang::glslang-default-resource-limits
            glslang::SPIRV
    )
    target_compile_definitions(oglapphelpers
        PUBLIC
            OCIO_VULKAN_ENABLED
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
