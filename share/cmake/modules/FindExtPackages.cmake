# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# All find modules used in this module support typical find_package
# behavior or installation of packages from external projects, as configured 
# by the OCIO_INSTALL_EXT_PACKAGES option.
#

# expat
# https://github.com/libexpat/libexpat
find_package(expat 2.2.8 REQUIRED)

# yaml-cpp
# https://github.com/jbeder/yaml-cpp
find_package(yaml-cpp 0.6.3 REQUIRED)

# Half (OpenEXR/IlmBase)
# https://github.com/openexr/openexr
find_package(Half 2.4.0 REQUIRED)

# pystring
# https://github.com/imageworks/pystring
find_package(pystring 1.1.3 REQUIRED)

if(OCIO_BUILD_APPS)
    # lcms2
    # https://github.com/mm2/Little-CMS
    find_package(lcms2 2.2 REQUIRED)
endif()

if(OCIO_BUILD_PYTHON)
    # pybind11
    # https://github.com/pybind/pybind11
    find_package(pybind11 2.4.3 REQUIRED)
endif()

if(OCIO_BUILD_DOCS)
    find_package(Python QUIET COMPONENTS Interpreter)

    if(Python_Interpreter_FOUND)
        if(Python_VERSION_MAJOR GREATER_EQUAL 3)
            set(Sphinx_MIN_VERSION 2.0.0)
        else()
            # Last release with Python 2.7 support
            set(Sphinx_MIN_VERSION 1.8.5)
        endif()

        # Sphinx
        # https://pypi.python.org/pypi/Sphinx
        find_package(Sphinx ${Sphinx_MIN_VERSION} REQUIRED)
    endif()
endif()
