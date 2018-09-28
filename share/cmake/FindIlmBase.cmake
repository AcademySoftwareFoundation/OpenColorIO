#
# Variables defined by this module:
#   ILMBASE_FOUND
#   ILMBASE_INCLUDE_DIR
#
# Usage: 
#   FIND_PACKAGE( ILMBASE )
#   FIND_PACKAGE( ILMBASE REQUIRED )
#

find_path(ILMBASE_INCLUDE_DIR OpenEXR/half.h PATH_SUFFIXES include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ILMBASE REQUIRED_VARS ILMBASE_INCLUDE_DIR)
mark_as_advanced(ILMBASE_INCLUDE_DIR)
