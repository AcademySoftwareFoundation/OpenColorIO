# Find the tinyxml XML parsing library.
#
# Sets the usual variables expected for find_package scripts:
#
#  TINYXML_INCLUDE_DIRS - header location
#  TINYXML_LIBRARIES - library to link against
#  TINYXML_FOUND - true if tinyxml was found.
#  TINYXML_MAJOR_VERSION
#  TINYXML_MINOR_VERSION
#  TINYXML_PATCH_VERSION
#  TINYXML_VERSION 

find_path(TINYXML_INCLUDE_DIR tinyxml.h)
find_library(TINYXML_LIBRARY NAMES tinyxml)

# Try to get the tinyxml version from the header file.
if(TINYXML_INCLUDE_DIR)
    set(_tixml_header ${TINYXML_INCLUDE_DIR}/tinyxml.h)
    file(READ ${_tixml_header} _contents)
    if(_contents)
        string(REGEX MATCH "const int TIXML_MAJOR_VERSION = ([0-9]+);" _TMP_major "${_contents}")
        string(REGEX REPLACE ".*([0-9]+).*" "\\1" _OUT_major "${_TMP_major}")
        string(REGEX MATCH "const int TIXML_MINOR_VERSION = ([0-9]+);" _TMP_minor "${_contents}")
        string(REGEX REPLACE ".*([0-9]+).*" "\\1" _OUT_minor "${_TMP_minor}")
        string(REGEX MATCH "const int TIXML_PATCH_VERSION = ([0-9]+);" _TMP_patch "${_contents}")
        string(REGEX REPLACE ".*([0-9]+).*" "\\1" _OUT_patch "${_TMP_patch}")

        if(NOT ${_OUT_major} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for TIXML_MAJOR_VERSION!")
        endif()
        if(NOT ${_OUT_minor} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for TIXML_MINOR_VERSION!")
        endif()
        if(NOT ${_OUT_patch} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for TIXML_MICRO_VERSION!")
        endif()

        set(TINYXML_MAJOR_VERSION ${_OUT_major})
        set(TINYXML_MINOR_VERSION ${_OUT_minor})
        set(TINYXML_PATCH_VERSION ${_OUT_patch})
        set(TINYXML_VERSION ${TINYXML_MAJOR_VERSION}.${TINYXML_MINOR_VERSION}.${TINYXML_PATCH_VERSION})
    else()
        message(FATAL_ERROR "Include file ${_tixml_header} does not exist")
    endif()
endif()

# Support the REQUIRED and QUIET arguments, and set TINYXML_FOUND if found.
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (TinyXML DEFAULT_MSG TINYXML_LIBRARY
                                   TINYXML_INCLUDE_DIR)

message(STATUS "TinyXML version: ${TINYXML_VERSION}")

if(TINYXML_FOUND)
    set(TINYXML_LIBRARIES ${TINYXML_LIBRARY})
    set(TINYXML_INCLUDE_DIRS ${TINYXML_INCLUDE_DIR})
endif()

mark_as_advanced(TINYXML_LIBRARY TINYXML_INCLUDE_DIR)
