# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(OCIO_BUILD_TESTS OR OCIO_BUILD_GPU_TESTS OR OCIO_BUILD_PYTHON)
    # Copy test files if used.
    add_subdirectory(data)
endif()

if(OCIO_BUILD_TESTS)
    # Test order is from low-level (i.e. no dependencies)
    # to high-level layers.
    add_subdirectory(testutils)
    add_subdirectory(utils)
    add_subdirectory(cpu)
    add_subdirectory(cmake-consumer)
endif()

if(OCIO_BUILD_GPU_TESTS)
    add_subdirectory(gpu)
endif()

if(TARGET OpenImageIO::OpenImageIO)

    # Running the OSL unit tests requires OSL which imposes OIIO and Imath.

    # OpenShadingLanguage
    # https://github.com/AcademySoftwareFoundation/OpenShadingLanguage
    find_package(OpenShadingLanguage 1.11.15)
    if(NOT OSL_FOUND)
        message(WARNING "Could NOT find OpenShadingLanguage. Skipping build the OSL unit tests")
    endif()
 
    if(OCIO_USE_OPENEXR_HALF)
        message(WARNING "Supported OpenShadingLanguage versions need Imath. Skipping build the OSL unit tests")
    endif()

    if(OSL_FOUND AND NOT OCIO_USE_OPENEXR_HALF)
        add_subdirectory(osl)
    endif()
else()
    message(WARNING "Could NOT find OpenImageIO. Skipping build the OSL unit tests")
endif()

if(OCIO_BUILD_JAVA)
    add_subdirectory(java)
endif()

if(OCIO_BUILD_PYTHON)
    add_subdirectory(python)
endif()
