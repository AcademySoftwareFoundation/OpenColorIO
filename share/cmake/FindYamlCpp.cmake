# Locate yaml-cpp
#
# This module defines
#  YAMLCPP_FOUND, if false, do not try to link to yaml-cpp
#  YAMLCPP_LIBRARY, where to find yaml-cpp
#  YAMLCPP_INCLUDE_DIR, where to find yaml.h
#  YAMLCPP_VERSION, the version of the library
#
# By default, the dynamic libraries of yaml-cpp will be found. To find the static ones instead,
# you must set the YAMLCPP_STATIC_LIBRARY variable to TRUE before calling find_package(YamlCpp ...).
#
# If yaml-cpp is not installed in a standard path, you can use the YAMLCPP_DIR CMake variable
# to tell CMake where yaml-cpp is.
include(FindPkgConfig)

# attempt to find static library first if this is set
if(YAMLCPP_STATIC_LIBRARY)
    set(YAMLCPP_STATIC libyaml-cpp.a libyaml-cppmd.lib)
endif()

# Try to use pkgconfig to get the verison
pkg_check_modules(PC_YAML_CPP REQUIRED QUIET yaml-cpp)

# find the yaml-cpp include directory
find_path(YAMLCPP_INCLUDE_DIR yaml-cpp/yaml.h
          HINTS
          ~/Library/Frameworks/yaml-cpp/include/
          /Library/Frameworks/yaml-cpp/include/
          /usr/local/include/
          /usr/include/
          /sw/yaml-cpp/         # Fink
          /opt/local/yaml-cpp/  # DarwinPorts
          /opt/csw/yaml-cpp/    # Blastwave
          /opt/yaml-cpp/
          ${PC_YAML_CPP_INCLUDEDIR}
          ${PC_YAML_CPP_INCLUDE_DIRS}
          PATH_SUFFIXES include
)

# find the yaml-cpp library
find_library(YAMLCPP_LIBRARY
             NAMES ${YAMLCPP_STATIC} yaml-cpp
             HINTS ~/Library/Frameworks
                    /Library/Frameworks
                    /usr/local
                    /usr
                    /sw
                    /opt/local
                    /opt/csw
                    /opt
                    ${PC_YAML_CPP_LIBRARY_DIRS}
             PATH_SUFFIXES lib64 lib
)

set(YAMLCPP_VERSION ${PC_YAML_CPP_VERSION})

# handle the QUIETLY and REQUIRED arguments and set YAMLCPP_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(YAMLCPP REQUIRED_VARS YAMLCPP_INCLUDE_DIR YAMLCPP_LIBRARY VERSION_VAR YAMLCPP_VERSION)
mark_as_advanced(YAMLCPP_LIBRARY YAMLCPP_INCLUDE_DIR YAMLCPP_VERSION)

