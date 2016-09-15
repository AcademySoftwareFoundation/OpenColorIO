# Find the LCMS library.
#
# Sets the usual variables expected for find_package scripts:
#
#  LCMS_INCLUDE_DIRS - header location
#  LCMS_LIBRARIES - library to link against
#  LCMS_FOUND - true if LCMS was found.

find_path(LCMS_INCLUDE_DIR lcms2.h)
find_library(LCMS_LIBRARY NAMES lcms2_static)

# Support the REQUIRED and QUIET arguments, and set LCMS_FOUND if found.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LCMS
    DEFAULT_MSG
    LCMS_LIBRARY LCMS_INCLUDE_DIR)

#  TODO: Infer the version string from the LCMS installation instead.
set(LCMS_VERSION "0.0.0" CACHE STRING "The version of the LCMS Library")

if(LCMS_FOUND)
    set(LCMS_LIBRARIES ${LCMS_LIBRARY})
    set(LCMS_INCLUDE_DIRS ${LCMS_INCLUDE_DIR})
endif()

mark_as_advanced(LCMS_LIBRARY LCMS_INCLUDE_DIR)
