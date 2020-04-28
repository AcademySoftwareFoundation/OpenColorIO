# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Variables defined by this module:
#   OpenImageIO_FOUND
#
# Targets exported by this module:
# OpenImageIO
#
# Usage: 
#   FIND_PACKAGE( OpenImageIO )
#   FIND_PACKAGE( OpenImageIO REQUIRED )
#

find_path(OpenImageIO_INCLUDE_DIR OpenImageIO/imageio.h PATH_SUFFIXES include)
find_library(OpenImageIO_LIBRARIES NAMES OpenImageIO OIIO)
if(OpenImageIO_LIBRARIES)
	get_filename_component(OpenImageIO_LIBRARY_DIR ${OpenImageIO_LIBRARIES} DIRECTORY)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenImageIO REQUIRED_VARS OpenImageIO_LIBRARIES OpenImageIO_INCLUDE_DIR OpenImageIO_LIBRARY_DIR)
mark_as_advanced(OpenImageIO_LIBRARIES OpenImageIO_INCLUDE_DIR OpenImageIO_LIBRARY_DIR)

if(OpenImageIO_FOUND)
	# Due to OpenImageIO exposing IlmBase includes in its public headers, we must add the IlmBase Include dir
	# to our include path to use OpenImageIO
	find_package(IlmBase REQUIRED)

	add_library(OpenImageIO INTERFACE IMPORTED GLOBAL)
	set(OpenImageIO_COMBINED_INCLUDES "")
	list(APPEND OpenImageIO_COMBINED_INCLUDES ${OpenImageIO_INCLUDE_DIR})
	list(APPEND OpenImageIO_COMBINED_INCLUDES ${ILMBASE_INCLUDE_DIR})
	set_property(TARGET OpenImageIO PROPERTY INTERFACE_LINK_LIBRARIES ${OpenImageIO_LIBRARIES})
	set_property(TARGET OpenImageIO PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OpenImageIO_COMBINED_INCLUDES})
endif()
