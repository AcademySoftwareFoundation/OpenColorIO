# Locate lcms2
#
# This module defines
#  LCMS2_FOUND, if false, do not try to link to lcms2
#  LCMS2_LIBRARY, where to find lcms2
#  LCMS2_INCLUDE_DIR, where to find lcms2.h
#  LCMS2_VERSION, the version of the library
#
# By default, the dynamic libraries of lcms2 will be found. To find the static ones instead,
# you must set the LCMS2_STATIC_LIBRARY variable to TRUE before calling find_package(LCMS2 ...).
#
include(FindPkgConfig)

# attempt to find static library first if this is set
if(LCMS2_STATIC_LIBRARY)
	set(LCMS2_STATIC liblcms2.a lcms2.lib)
endif()

# Try to use pkgconfig to get the verison
pkg_check_modules(PC_LCMS2 QUIET lcms2)

# find the lcms2 include directory
find_path(LCMS2_INCLUDE_DIR lcms2.h
		  HINTS
		  ~/Library/Frameworks/include/
		  /Library/Frameworks/include/
		  /usr/local/include/
		  /usr/include/
		  /sw/         # Fink
		  /opt/local/  # DarwinPorts
		  /opt/csw/    # Blastwave
		  /opt/
		  ${PC_LCMS2_INCLUDEDIR}
		  ${PC_LCMS2_INCLUDE_DIRS}
		  PATH_SUFFIXES include
)

# find the lcms2 library
find_library(LCMS2_LIBRARY
	NAMES ${LCMS2_STATIC} lcms2
			 HINTS ~/Library/Frameworks
					/Library/Frameworks
					/usr/local
					/usr
					/sw
					/opt/local
					/opt/csw
					/opt
					${PC_LCMS2_LIBRARY_DIRS}
			 PATH_SUFFIXES lib64 lib
)
if (PC_LCMS2_FOUND)
	set(LCMS2_VERSION "${PC_LCMS2_VERSION}")
else()
	set(_lcms2_header ${LCMS2_INCLUDE_DIR}/lcms2.h)
	file(READ ${_lcms2_header} _contents)
	string(REGEX MATCH "// Version ([0-9]\\.[0-9])" LCMS2_VERSION "${_contents}")
	string(REGEX REPLACE ".*([0-9]\\.[0-9]).*" "\\1" LCMS2_VERSION "${LCMS2_VERSION}")
	
	if(NOT LCMS2_VERSION)
		message(FATAL_ERROR "Error extracting version from ${_lcms2_header}")
	endif()
endif()

# handle the QUIETLY and REQUIRED arguments and set LCMS2_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LCMS2 REQUIRED_VARS LCMS2_INCLUDE_DIR LCMS2_LIBRARY VERSION_VAR LCMS2_VERSION)
mark_as_advanced(LCMS2_LIBRARY LCMS2_INCLUDE_DIR LCMS2_VERSION)

