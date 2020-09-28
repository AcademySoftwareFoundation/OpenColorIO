# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# Define variables around the GL support.

include(PackageUtils)

if(OCIO_BUILD_GPU_TESTS OR OCIO_BUILD_APPS)
    set(OCIO_GL_ENABLED ON)
    set(OCIO_USE_GLVND OFF)
    set(OCIO_EGL_HEADLESS OFF)

    find_package(OpenGL COMPONENTS OpenGL)
    if(NOT OpenGL_OpenGL_FOUND AND NOT OPENGL_GLU_FOUND)
        package_root_message(OpenGL)
        set(OCIO_GL_ENABLED OFF)
    endif()

    if(NOT APPLE)
        # On some Linux platform, the glew-config.cmake is found first so make it explicit
        # to fall back on the regular search if not found.
        find_package(GLEW CONFIG QUIET)
        if(NOT GLEW_FOUND)
            find_package(GLEW)
            if(NOT GLEW_FOUND)
                package_root_message(GLEW)
                set(OCIO_GL_ENABLED OFF)
            endif()
        else()
            # Expected variables GLEW_LIBRARIES and GLEW_INCLUDE_DIRS are missing so create
            # the mandatory one. Note that the cmake bug is now fixed (issue 19662).
            if(NOT GLEW_LIBRARIES)
                set(GLEW_LIBRARIES GLEW::GLEW)
            endif()
        endif()
    endif()

    find_package(GLUT)
    if(NOT GLUT_FOUND)
        package_root_message(GLUT)
        set(OCIO_GL_ENABLED OFF)
    endif()

    if(NOT OCIO_GL_ENABLED)
        message(WARNING "GPU rendering disabled")
    else()
        # OpenGL_egl_Library is defined iff GLVND is supported (CMake 10+).
        if(OPENGL_egl_LIBRARY)
            message(STATUS "GLVND supported")
            set(OCIO_USE_GLVND ON)
        else()
            message(STATUS "GLVND not supported; legacy OpenGL libraries used")
        endif()

        if(OCIO_USE_HEADLESS)
            if(CMAKE_SYSTEM_NAME STREQUAL Linux)
                if(NOT OCIO_USE_GLVND)
                    message(STATUS "Can't find EGL without GLVND support; can't render headlessly")
                    set(OCIO_USE_HEADLESS OFF)
                else()
                    find_package(OpenGL COMPONENTS EGL)
                    if(NOT OpenGL_EGL_FOUND)
                        message(WARNING "EGL component missing; can't render headlessly")
                        set(OCIO_USE_HEADLESS OFF)
                    else()
                        add_compile_definitions(OCIO_HEADLESS_ENABLED)
                        set(OCIO_EGL_HEADLESS ON)
                        message(STATUS "EGL enabled")
                    endif()
                endif()
            else()
                message(WARNING "OS system is not Linux; can't render headlessly")
            endif()
        endif()
    endif()
endif()


# An advanced variable will not be displayed in any of the cmake GUIs

mark_as_advanced(OCIO_GL_ENABLED)
mark_as_advanced(OCIO_USE_GLVND)
mark_as_advanced(OCIO_EGL_HEADLESS)
