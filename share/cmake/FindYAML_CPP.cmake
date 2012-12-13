# Find the YAML-CPP library.
#
# Sets the usual variables expected for find_package scripts:
#
#  YAML_CPP_INCLUDE_DIRS - header location
#  YAML_CPP_LIBRARIES - library to link against
#  YAML_CPP_FOUND - true if YAML_CPP was found.

find_path(YAML_CPP_INCLUDE_DIR yaml-cpp/yaml.h)
find_library(YAML_CPP_LIBRARY NAMES yaml-cpp)

# Support the REQUIRED and QUIET arguments, and set YAML_CPP_FOUND if found.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    YAML_CPP
    DEFAULT_MSG
    YAML_CPP_LIBRARY YAML_CPP_INCLUDE_DIR)

#  TODO: Infer the version string from the YAMLCPP installation instead.
set(YAML_CPP_VERSION "0.0.0" CACHE STRING "The version of the YAMLCPP Library")

if(YAML_CPP_FOUND)
    set(YAML_CPP_LIBRARIES ${YAML_CPP_LIBRARY})
    set(YAML_CPP_INCLUDE_DIRS ${YAML_CPP_INCLUDE_DIR})
endif()

mark_as_advanced(YAML_CPP_LIBRARY YAML_CPP_INCLUDE_DIR)
