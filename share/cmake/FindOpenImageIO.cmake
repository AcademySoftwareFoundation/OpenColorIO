#
# Variables defined by this module:
#   OIIO_FOUND    
#   OIIO_INCLUDE_DIR
#   OIIO_LIBRARIES
#   OIIO_LIBRARY_DIR
#
# Usage: 
#   FIND_PACKAGE( OIIO )
#   FIND_PACKAGE( OIIO REQUIRED )
#

find_path(OIIO_INCLUDE_DIR OpenImageIO/imageio.h PATH_SUFFIXES include)
find_library(OIIO_LIBRARIES NAMES OIIO OpenImageIO)
if(OIIO_LIBRARIES)
	get_filename_component(OIIO_LIBRARY_DIR ${OIIO_LIBRARIES} DIRECTORY)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OIIO REQUIRED_VARS OIIO_LIBRARIES OIIO_INCLUDE_DIR OIIO_LIBRARY_DIR)
mark_as_advanced(OIIO_LIBRARIES OIIO_INCLUDE_DIR OIIO_LIBRARY_DIR)
