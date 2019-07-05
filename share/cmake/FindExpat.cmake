# Find Expat
#
# Variables defined by this module:
#   EXPAT_FOUND
#   EXPAT_INCLUDE_DIRS
#   EXPAT_LIBRARY
#   EXPAT_VERSION
#
# Usage:
#   find_package(Expat [version] [REQUIRED])
#
# Note:
# You can tell the module where Expat is installed by setting any of 
# these variable before calling find_package:
#   EXPAT_INCLUDE_DIR - Directory containing expat/expat.h
#   EXPAT_LIBRARY_DIR - Directory containing expat library
#   EXPAT_STATIC_LIBRARY - Prefer static library
#

include(FindPkgConfig)
pkg_check_modules(PC_EXPAT QUIET expat)

set(EXPAT_VERSION ${PC_EXPAT_VERSION})

if(NOT EXPAT_INCLUDE_DIR)
    find_path(EXPAT_INCLUDE_DIR "expat/expat.h"
              HINTS ${EXT_INCLUDE_DIR}
                    ${PC_EXPAT_INCLUDEDIR}
                    ${PC_EXPAT_INCLUDE_DIRS}
              PATH_SUFFIXES include
    )
endif()

if (NOT EXPAT_LIBRARY)
    if(EXISTS "${EXT_INCLUDE_DIR}/expat" OR EXPAT_STATIC_LIBRARY)
        set(EXPAT_STATIC libexpat.a libexpat.lib)
    endif()

    find_library(EXPAT_LIBRARY 
                 NAMES ${EXPAT_STATIC} expat
                 HINTS ${EXPAT_LIBRARY_DIR}
                       ${EXT_LIBRARY_DIR}
                       ${PC_EXPAT_LIBDIR}
                       ${PC_EXPAT_LIBRARY_DIRS}
                 PATH_SUFFIXES lib lib64
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Expat
    REQUIRED_VARS EXPAT_INCLUDE_DIR
                  EXPAT_LIBRARY
    VERSION_VAR EXPAT_VERSION
)
mark_as_advanced(EXPAT_INCLUDE_DIR EXPAT_LIBRARY EXPAT_VERSION)

set(EXPAT_INCLUDE_DIRS ${EXPAT_INCLUDE_DIR})

