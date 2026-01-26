# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# Define a function to set debugger environment so that the run time 
# dependencies can be located by the debugger.

function(set_debugger_env target_name)
    cmake_parse_arguments(ARG "NEEDS_GL" "" "" ${ARGN})

    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "set_debugger_env: '${target_name}' is not a CMake target")
    endif()

    # Set the Paths for Visual Studio IDE Debugger.
    if(MSVC)
        if(OCIO_GL_ENABLED AND ARG_NEEDS_GL)
            # Add folders for glut and glew DLLs.
            set(extra_dirs "${GLUT_INCLUDE_DIR}/../bin;${GLEW_INCLUDE_DIRS}/../bin")
        endif()

        set_property(TARGET ${target_name} PROPERTY
            VS_DEBUGGER_ENVIRONMENT "PATH=$<JOIN:$<TARGET_RUNTIME_DLL_DIRS:${target_name}>,;>;${extra_dirs};%PATH%"
        )
    endif()
endfunction()