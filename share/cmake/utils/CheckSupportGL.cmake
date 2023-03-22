# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# Define variables around the GL support.

include(PackageUtils)
include(SelectLibraryConfigurations)
include(Colors)
include(ocio_handle_dependency)

if((OCIO_BUILD_TESTS AND OCIO_BUILD_GPU_TESTS) OR OCIO_BUILD_APPS)
    set(OCIO_GL_ENABLED ON)
    set(OCIO_USE_GLVND OFF)
    set(OCIO_EGL_HEADLESS OFF)

    ocio_handle_dependency(OpenGL COMPONENTS OpenGL)
    if(NOT OpenGL_OpenGL_FOUND AND NOT OPENGL_GLU_FOUND)
        package_root_message(OpenGL)
        set(OCIO_GL_ENABLED OFF)
    endif()

    if(NOT APPLE)
        # On some Linux platform, the glew-config.cmake is found first.
        # PREFER_CONFIG assure that config mode will be used first and fallback to module mode.
        ocio_handle_dependency(  GLEW PREFER_CONFIG
                                 MIN_VERSION 2.1.0
                                 RECOMMENDED_VERSION 2.1.0
                                 RECOMMENDED_VERSION_REASON "Latest version tested with OCIO")
        if(NOT GLEW_FOUND)
            package_root_message(GLEW)
            set(OCIO_GL_ENABLED OFF)
        else()
            # Expected variables GLEW_LIBRARIES and GLEW_INCLUDE_DIRS are missing so create
            # the mandatory one.
            if(NOT GLEW_LIBRARIES)
                set(GLEW_LIBRARIES GLEW::GLEW)
            endif()
        endif()

        # Cmake has a bug with FindGLEW. It fails to defined GLEW_INCLUDE_DIRS in some situation.
        # See https://gitlab.kitware.com/cmake/cmake/-/issues/19662
        # See a closed duplicate of 19662: https://gitlab.kitware.com/cmake/cmake/-/issues/20699
        # Ported from vcpkg Glew package - https://github.com/microsoft/vcpkg/blob/master/ports/glew/vcpkg-cmake-wrapper.cmake
        # The following code make sure that GLEW_INCLUDE_DIRS is set correctly.
        if(MSVC AND GLEW_FOUND AND TARGET GLEW::GLEW AND NOT DEFINED GLEW_INCLUDE_DIRS)
            get_target_property(GLEW_INCLUDE_DIRS GLEW::GLEW INTERFACE_INCLUDE_DIRECTORIES)
            set(GLEW_INCLUDE_DIR ${GLEW_INCLUDE_DIRS})
            get_target_property(_GLEW_DEFS GLEW::GLEW INTERFACE_COMPILE_DEFINITIONS)
            if("${_GLEW_DEFS}" MATCHES "GLEW_STATIC")
                get_target_property(GLEW_LIBRARY_DEBUG GLEW::GLEW IMPORTED_LOCATION_DEBUG)
                get_target_property(GLEW_LIBRARY_RELEASE GLEW::GLEW IMPORTED_LOCATION_RELEASE)
            else()
                get_target_property(GLEW_LIBRARY_DEBUG GLEW::GLEW IMPORTED_IMPLIB_DEBUG)
                get_target_property(GLEW_LIBRARY_RELEASE GLEW::GLEW IMPORTED_IMPLIB_RELEASE)
            endif()
            get_target_property(_GLEW_LINK_INTERFACE GLEW::GLEW IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE) # same for debug and release
            list(APPEND GLEW_LIBRARIES ${_GLEW_LINK_INTERFACE})
            list(APPEND GLEW_LIBRARY ${_GLEW_LINK_INTERFACE})
            select_library_configurations(GLEW)
            if("${_GLEW_DEFS}" MATCHES "GLEW_STATIC")
                set(GLEW_STATIC_LIBRARIES ${GLEW_LIBRARIES})
            else()
                set(GLEW_SHARED_LIBRARIES ${GLEW_LIBRARIES})
            endif()
            unset(_GLEW_DEFS)
            unset(_GLEW_LINK_INTERFACE)
        endif()
    endif()

    ocio_handle_dependency(  GLUT
                             MIN_VERSION 3.2.1
                             RECOMMENDED_VERSION 3.2.1
                             RECOMMENDED_VERSION_REASON "Latest version tested with OCIO")
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
                    ocio_handle_dependency(OpenGL COMPONENTS EGL)
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
