###############################################################################
### External C/C++ dependencies ###

# Expat
# https://libexpat.github.io
set(EXPAT_MIN_VERSION 2.2.5)
find_package(Expat ${EXPAT_MIN_VERSION})

# yaml-cpp
# https://github.com/jbeder/yaml-cpp
set(YAMLCPP_MIN_VERSION 0.3.0)
find_package(YamlCpp ${YAMLCPP_MIN_VERSION})

# LCMS2
# http://www.littlecms.com
set(LCMS2_MIN_VERSION 2.2)
find_package(LCMS2 ${LCMS2_MIN_VERSION})

# IlmBase (Half part only)
# https://www.openexr.com
set(ILMBASE_MIN_VERSION 2.3.0)
find_package(IlmBase ${ILMBASE_MIN_VERSION})

# pystring
# https://github.com/imageworks/pystring
set(PYSTRING_MIN_VERSION 1.1.3)
find_package(Pystring ${PYSTRING_MIN_VERSION})


###############################################################################
### sampleicc ###
add_library(sampleicc INTERFACE IMPORTED GLOBAL)
set_property(TARGET sampleicc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/sampleicc/src/include")
