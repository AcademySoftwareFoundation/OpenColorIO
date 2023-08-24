# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Check for compatibility between OpenEXR and OpenImageIO since OCIO requires OpenEXR 3+.
#

message(STATUS "Checking if the OpenImageIO found is built with OpenEXR 3+...")

find_path (OpenImageIO_INCLUDE_DIR
    NAMES
        OpenImageIO/imageio.h
    HINTS
        ${OpenImageIO_ROOT}
        # Assuming that OpenImageIO was installed normally, go back a few folders down
        # to get the equivalent of OpenImageIO_ROOT.
        ${OpenImageIO_DIR}/../../..
    PATH_SUFFIXES
        OpenImageIO/include
        include
)

if (NOT OpenImageIO_INCLUDE_DIR)
    message(STATUS "${ColorWarning}Could not find OpenImageIO header to evaluate the OpenEXR version.")
    message(STATUS "Please provide the OpenImageIO_DIR variable.")
    message(STATUS "If your OpenImageIO's files are located in different root directory, \
please provide the OpenImageIO_ROOT where the include files are located.${ColorReset}")
endif()

# Try to figure out version number
set (OIIO_VERSION_HEADER "${OpenImageIO_INCLUDE_DIR}/OpenImageIO/oiioversion.h")
if (EXISTS "${OIIO_VERSION_HEADER}")
    file (STRINGS "${OIIO_VERSION_HEADER}" TMP REGEX "^#define OIIO_VERSION_MAJOR .*$")
    string (REGEX MATCHALL "[0-9]+" OpenImageIO_VERSION_MAJOR ${TMP})
    file (STRINGS "${OIIO_VERSION_HEADER}" TMP REGEX "^#define OIIO_VERSION_MINOR .*$")
    string (REGEX MATCHALL "[0-9]+" OpenImageIO_VERSION_MINOR ${TMP})
    file (STRINGS "${OIIO_VERSION_HEADER}" TMP REGEX "^#define OIIO_VERSION_PATCH .*$")
    string (REGEX MATCHALL "[0-9]+" OpenImageIO_VERSION_PATCH ${TMP})
    file (STRINGS "${OIIO_VERSION_HEADER}" TMP REGEX "^#define OIIO_VERSION_TWEAK .*$")
    if (TMP)
        string (REGEX MATCHALL "[0-9]+" OpenImageIO_VERSION_TWEAK ${TMP})
    else ()
        set (OpenImageIO_VERSION_TWEAK 0)
    endif ()
    set (OpenImageIO_VERSION "${OpenImageIO_VERSION_MAJOR}.${OpenImageIO_VERSION_MINOR}.${OpenImageIO_VERSION_PATCH}.${OpenImageIO_VERSION_TWEAK}")
endif ()

set (OIIO_IMATH_HEADER "${OpenImageIO_INCLUDE_DIR}/OpenImageIO/Imath.h")
if (EXISTS "${OIIO_IMATH_HEADER}")
    file(STRINGS "${OIIO_IMATH_HEADER}" TMP REGEX "^#define OIIO_USING_IMATH .*$")
    string(REGEX MATCHALL "[0-9]" OIIO_IMATH_VERSION ${TMP})
    if (OIIO_IMATH_VERSION LESS 3)
        message(STATUS "Skipping OpenImageIO built against OpenEXR 2, please use version 3 or greater.")
    else()
        set(is_OpenEXR_VERSION_valid TRUE)
    endif()
endif()

# clean up variables
unset(OpenImageIO_INCLUDE_DIR)
unset(OIIO_VERSION_HEADER)
unset(OIIO_VERSION_MAJOR)
unset(OIIO_VERSION_MINOR)
unset(OIIO_VERSION_PATCH)
unset(OIIO_VERSION_TWEAK)
unset(OIIO_IMATH_HEADER)
unset(OIIO_IMATH_VERSION)