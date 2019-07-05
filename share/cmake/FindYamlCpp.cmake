# Find YAMLCPP
#
# Variables defined by this module:
#   YAMLCPP_FOUND
#   YAMLCPP_INCLUDE_DIRS
#   YAMLCPP_LIBRARY
#   YAMLCPP_VERSION
#
# Usage:
#   find_package(YAMLCPP [version] [REQUIRED])
#
# Note:
# You can tell the module where YAMLCPP is installed by setting any of 
# these variable before calling find_package:
#   YAMLCPP_INCLUDE_DIR - Directory containing yaml-cpp/yaml.h
#   YAMLCPP_LIBRARY_DIR - Directory containing yaml-cpp library
#   YAMLCPP_STATIC_LIBRARY - Prefer static library
#

include(FindPkgConfig)
pkg_check_modules(PC_YAMLCPP QUIET yaml-cpp)

set(YAMLCPP_VERSION ${PC_YAMLCPP_VERSION})

if(NOT YAMLCPP_INCLUDE_DIR)
    find_path(YAMLCPP_INCLUDE_DIR yaml-cpp/yaml.h
              HINTS ${EXT_INCLUDE_DIR}
                   ~/Library/Frameworks/yaml-cpp/include/
                    /Library/Frameworks/yaml-cpp/include/
                    /usr/local/include/
                    /usr/include/
                    /sw/yaml-cpp/         # Fink
                    /opt/local/yaml-cpp/  # DarwinPorts
                    /opt/csw/yaml-cpp/    # Blastwave
                    /opt/yaml-cpp/
                    ${PC_YAMLCPP_INCLUDEDIR}
                    ${PC_YAMLCPP_INCLUDE_DIRS}
              PATH_SUFFIXES include
    )
endif()

if (NOT YAMLCPP_LIBRARY)
    if(EXISTS "${EXT_INCLUDE_DIR}/yaml-cpp" OR YAMLCPP_STATIC_LIBRARY)
        set(YAMLCPP_STATIC libyaml-cpp.a libyaml-cppmd.lib)
    endif()

    find_library(YAMLCPP_LIBRARY
                 NAMES ${YAMLCPP_STATIC} yaml-cpp
                 HINTS ${YAMLCPP_LIBRARY_DIR}
                       ${EXT_LIBRARY_DIR}
                      ~/Library/Frameworks
                       /Library/Frameworks
                       /usr/local
                       /usr
                       /sw
                       /opt/local
                       /opt/csw
                       /opt
                       ${PC_YAMLCPP_LIBDIRS}
                       ${PC_YAMLCPP_LIBRARY_DIRS}
                 PATH_SUFFIXES lib64 lib
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(YAMLCPP
    REQUIRED_VARS YAMLCPP_INCLUDE_DIR 
                  YAMLCPP_LIBRARY 
    VERSION_VAR YAMLCPP_VERSION
)
mark_as_advanced(YAMLCPP_LIBRARY YAMLCPP_INCLUDE_DIR YAMLCPP_VERSION)

set(YAMLCPP_INCLUDE_DIRS ${YAMLCPP_INCLUDE_DIR})
