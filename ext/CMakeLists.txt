# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
### External third-party C/C++ dependencies.                                ###

# Expat
# https://github.com/libexpat/libexpat
set(EXPAT_MIN_VERSION 2.2.5)
find_package(Expat ${EXPAT_MIN_VERSION} REQUIRED)

# yaml-cpp
# https://github.com/jbeder/yaml-cpp
set(YAMLCPP_MIN_VERSION 0.6.3)
find_package(YamlCpp ${YAMLCPP_MIN_VERSION} REQUIRED)

# IlmBase (Half part only)
# https://github.com/openexr/openexr
set(ILMBASE_MIN_VERSION 2.3.0)
find_package(IlmBase ${ILMBASE_MIN_VERSION} REQUIRED)

# Pystring
# https://github.com/imageworks/pystring
set(PYSTRING_MIN_VERSION 1.1.3)
find_package(Pystring ${PYSTRING_MIN_VERSION} REQUIRED)

if(OCIO_BUILD_APPS)
    # LCMS2
    # https://github.com/mm2/Little-CMS
    set(LCMS2_MIN_VERSION 2.2)
    find_package(LCMS2 ${LCMS2_MIN_VERSION} REQUIRED)
endif()

###############################################################################
### External (modified) third-party C/C++ dependencies.                     ###

# Sample ICC (modified)
add_subdirectory(sampleicc)
