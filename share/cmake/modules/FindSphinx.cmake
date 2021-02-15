# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate Sphinx (Python documentation generator)
#
# Variables defined by this module:
#   Sphinx_FOUND
#   Sphinx_EXECUTABLE (CACHE)
#
# Usage:
#   find_package(Sphinx)
#
# If Sphinx is not installed in a standard path, add it to the Sphinx_ROOT 
# variable to tell CMake where to find it.
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
