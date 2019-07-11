# Find Expat
#
# Variables defined by this module:
#   LIBEXPAT_FOUND
#   LIBEXPAT_INCLUDE_DIRS
#   LIBEXPAT_LIBRARY
#   LIBEXPAT_VERSION
#
# Usage:
#   find_package(Expat [version] [REQUIRED])
#
# Note:
# You can tell the module where Expat is installed by setting any of 
# these variable before calling find_package:
#   LIBEXPAT_INCLUDE_DIR - Directory containing expat/expat.h
#   LIBEXPAT_LIBRARY_DIR - Directory containing expat library
#   LIBEXPAT_STATIC_LIBRARY - Prefer static library
#

include(FindPkgConfig)
pkg_check_modules(PC_EXPAT QUIET expat)

set(LIBEXPAT_VERSION ${PC_LIBEXPAT_VERSION})

if(NOT LIBEXPAT_INCLUDE_DIR OR NOT EXISTS LIBEXPAT_INCLUDE_DIR)
    find_path(LIBEXPAT_INCLUDE_DIR "expat.h"
              HINTS ${EXT_INCLUDE_DIR}
                    ${PC_LIBEXPAT_INCLUDEDIR}
                    ${PC_LIBEXPAT_INCLUDE_DIRS}
              PATH_SUFFIXES include
    )
endif()

if (NOT LIBEXPAT_LIBRARY OR NOT EXISTS LIBEXPAT_LIBRARY)
    if(EXISTS "${EXT_INCLUDE_DIR}/expat.h" OR LIBEXPAT_STATIC_LIBRARY)
        set(LIBEXPAT_STATIC libexpat.a libexpat.lib)
    endif()

    find_library(LIBEXPAT_LIBRARY 
                 NAMES ${LIBEXPAT_STATIC} expat
                 HINTS ${LIBEXPAT_LIBRARY_DIR}
                       ${EXT_LIBRARY_DIR}
                       ${PC_LIBEXPAT_LIBDIR}
                       ${PC_LIBEXPAT_LIBRARY_DIRS}
                 PATH_SUFFIXES lib lib64
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBEXPAT
    REQUIRED_VARS LIBEXPAT_INCLUDE_DIR
                  LIBEXPAT_LIBRARY
    VERSION_VAR LIBEXPAT_VERSION
)
mark_as_advanced(LIBEXPAT_INCLUDE_DIR LIBEXPAT_LIBRARY LIBEXPAT_VERSION)

set(LIBEXPAT_INCLUDE_DIRS ${LIBEXPAT_INCLUDE_DIR})

