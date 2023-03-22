# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate Sphinx (Python documentation generator)
#
# Variables defined by this module:
#   Sphinx_FOUND             - Indicate whether the executable was found or not
#   Sphinx_EXECUTABLE        - Path to the executable file
#
#
# If the executable is not installed in a typical location where CMake will find it, you may specify 
# the location using one of the following methods:
# -- Set -DSphinx_ROOT to point to the directory containing the executable.
# -- Set -DSphinx_EXECUTABLE to point to executable file.
#

find_package(Python QUIET COMPONENTS Interpreter)

if(Python_Interpreter_FOUND AND WIN32)
    get_filename_component(_Python_ROOT "${Python_EXECUTABLE}" DIRECTORY)
    set(_Python_SCRIPTS_DIR "${_Python_ROOT}/Scripts")
endif()

# Find sphinx-build
find_program(Sphinx_EXECUTABLE 
    NAMES 
        sphinx-build
    HINTS
        ${Sphinx_ROOT}
        ${_Python_SCRIPTS_DIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sphinx
    REQUIRED_VARS 
        Sphinx_EXECUTABLE
)

mark_as_advanced(Sphinx_EXECUTABLE)
