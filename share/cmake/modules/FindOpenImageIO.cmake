#
# Variables defined by this module:
#   OIIO_FOUND
#
# Targets exported by this module:
# OpenImageIO
#
# Usage: 
#   FIND_PACKAGE( OpenImageIO )
#   FIND_PACKAGE( OpenImageIO REQUIRED )
#

find_path(OIIO_INCLUDE_DIR OpenImageIO/imageio.h PATH_SUFFIXES include)
find_library(OIIO_LIBRARIES NAMES OIIO OpenImageIO)
if(OIIO_LIBRARIES)
	get_filename_component(OIIO_LIBRARY_DIR ${OIIO_LIBRARIES} DIRECTORY)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OIIO REQUIRED_VARS OIIO_LIBRARIES OIIO_INCLUDE_DIR OIIO_LIBRARY_DIR)
mark_as_advanced(OIIO_LIBRARIES OIIO_INCLUDE_DIR OIIO_LIBRARY_DIR)

if(OIIO_FOUND)
	# Due to OpenImageIO exposing IlmBase includes in its public headers, we must add the IlmBase Include dir
	# to our include path to use OIIO
	find_package(IlmBase REQUIRED)

	add_library(OpenImageIO INTERFACE IMPORTED GLOBAL)
	set(OIIO_COMBINED_INCLUDES "")
	list(APPEND OIIO_COMBINED_INCLUDES ${OIIO_INCLUDE_DIR})
	list(APPEND OIIO_COMBINED_INCLUDES ${ILMBASE_INCLUDE_DIR})
	set_property(TARGET OpenImageIO PROPERTY INTERFACE_LINK_LIBRARIES ${OIIO_LIBRARIES})
	set_property(TARGET OpenImageIO PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OIIO_COMBINED_INCLUDES})
endif()
