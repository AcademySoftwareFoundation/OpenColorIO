# Find Sphinx (Python documentation generator)
#
# Variables defined by this module:
#   SPHINX_FOUND
#   SPHINX_EXECUTABLE
#
# Usage:
#   find_package(Sphinx)
#

find_program(SPHINX_EXECUTABLE 
             NAMES sphinx-build)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SPHINX SPHINX_EXECUTABLE)
mark_as_advanced(SPHINX_EXECUTABLE)
