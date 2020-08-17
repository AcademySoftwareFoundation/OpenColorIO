# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# All find modules used in this module support typical find_package
# behavior or installation of packages from external projects, as configured 
# by the OCIO_INSTALL_EXT_PACKAGES option.
#

###############################################################################
### Global package options ###

# Some packages register their CMake config location in the CMake User or 
# System Package Registry. We disable these search locations globally since 
# they can cause unwanted linking between multiple builds of OpenColorIO 
# when a package has previously been installed to ext/dist. Set these variables
# to OFF during cmake configuration to enable package registry use.

set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON CACHE BOOL "Disable CMake User Package Registry when finding packages")
set(CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY ON CACHE BOOL "Disable CMake System Package Registry when finding packages")

###############################################################################
### Packages and versions ###

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

        include(FindPythonPackage)

        # testresources
        # https://pypi.org/project/testresources/
        find_python_package(testresources 2.0.1 REQUIRED)

        # Sphinx Tabs
        # https://pypi.org/project/sphinx-tabs/
        find_python_package(sphinx-tabs 1.1.13 REQUIRED)

        # Recommonmark
        # https://pypi.org/project/recommonmark/
        find_python_package(recommonmark 0.6.0 REQUIRED)

        # Sphinx Press Theme
        # https://pypi.org/project/sphinx-press-theme/
        find_python_package(sphinx-press-theme 0.5.1 REQUIRES testresources REQUIRED)

    endif()
endif()
